#include "rai_open_ai_agent.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QUrl>
#include <QUuid>

#include "rbl_logger.h"

ROpenAiAgent::ROpenAiAgent(const ROpenAiAgentSettings &settings, QObject *parent)
    : QObject{parent}
    , settings{settings}
    , networkManager{new QNetworkAccessManager(this)}
    , networkReply{nullptr}
    , remainingRetries{0}
{
    if (!settings.proxyHost().isEmpty()) {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(settings.proxyHost());
        proxy.setPort(static_cast<quint16>(settings.proxyPort()));
        if (!settings.proxyUser().isEmpty()) {
            proxy.setUser(settings.proxyUser());
            proxy.setPassword(settings.proxyPassword());
        }
        networkManager->setProxy(proxy);
        RLogger::info("[ROpenAiAgent] Proxy configured: %s:%d\n",
                      settings.proxyHost().toUtf8().constData(),
                      settings.proxyPort());
    }

    networkManager->setTransferTimeout(settings.timeout());
    startNewSession();
}

ROpenAiAgent::~ROpenAiAgent()
{
    if (networkReply) {
        networkReply->abort();
        networkReply->deleteLater();
    }
}

QString ROpenAiAgent::getSessionId() const
{
    return sessionId;
}

void ROpenAiAgent::startNewSession()
{
    sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    conversationHistory = QJsonArray();
    RLogger::info("[ROpenAiAgent] New session started: %s\n",
                  sessionId.toUtf8().constData());
}

void ROpenAiAgent::chat(const QString &message)
{
    if (message.isEmpty()) {
        RLogger::warning("[ROpenAiAgent] chat() called with an empty message.\n");
        return;
    }

    if (networkReply && networkReply->isRunning()) {
        RLogger::warning("[ROpenAiAgent] A request is already in progress; ignoring new chat() call.\n");
        return;
    }

    pendingMessage = message;
    remainingRetries = settings.retryCount();
    sendRequest(message);
}

void ROpenAiAgent::sendRequest(const QString &message)
{
    // Build messages array: optional system prompt + confirmed history + current user turn.
    // The OpenAI API carries the system prompt as a message with role "system".
    QJsonArray messages;

    if (!settings.systemPrompt().isEmpty()) {
        QJsonObject systemMessage;
        systemMessage[QStringLiteral("role")]    = QStringLiteral("system");
        systemMessage[QStringLiteral("content")] = settings.systemPrompt();
        messages.append(systemMessage);
    }

    for (const QJsonValue &entry : std::as_const(conversationHistory)) {
        messages.append(entry);
    }

    QJsonObject userMessage;
    userMessage[QStringLiteral("role")]    = QStringLiteral("user");
    userMessage[QStringLiteral("content")] = message;
    messages.append(userMessage);

    QJsonObject body;
    body[QStringLiteral("model")]       = settings.model();
    body[QStringLiteral("max_tokens")]  = settings.maxTokens();
    body[QStringLiteral("messages")]    = messages;
    body[QStringLiteral("temperature")] = settings.temperature();
    body[QStringLiteral("top_p")]       = settings.topP();

    const QByteArray requestBody = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkRequest request{QUrl(settings.apiUrl())};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json"));
    request.setRawHeader(QByteArrayLiteral("Authorization"),
                         QByteArrayLiteral("Bearer ") + settings.apiKey().toUtf8());

    RLogger::info("[ROpenAiAgent] [session: %s] Sending request (turn %d, model: %s)\n",
                  sessionId.toUtf8().constData(),
                  static_cast<int>(conversationHistory.size() / 2 + 1),
                  settings.model().toUtf8().constData());

    networkReply = networkManager->post(request, requestBody);
    QObject::connect(networkReply, &QNetworkReply::finished, this, &ROpenAiAgent::onReplyFinished);
}

void ROpenAiAgent::onReplyFinished()
{
    const QNetworkReply::NetworkError networkError = networkReply->error();
    const int httpStatus =
        networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (networkError != QNetworkReply::NoError) {
        const QString errorString = networkReply->errorString();
        networkReply->deleteLater();
        networkReply = nullptr;

        if (remainingRetries > 0) {
            remainingRetries--;
            RLogger::warning("[ROpenAiAgent] [session: %s] Request failed (%s), retrying (%d attempt(s) left).\n",
                             sessionId.toUtf8().constData(),
                             errorString.toUtf8().constData(),
                             remainingRetries + 1);
            sendRequest(pendingMessage);
            return;
        }

        RLogger::error("[ROpenAiAgent] [session: %s] Request failed after all retries: %s\n",
                       sessionId.toUtf8().constData(),
                       errorString.toUtf8().constData());
        emit errorOccurred(errorString);
        return;
    }

    const QByteArray responseData = networkReply->readAll();
    networkReply->deleteLater();
    networkReply = nullptr;

    if (httpStatus != 200) {
        const QString errorString =
            QString("HTTP %1: %2").arg(httpStatus).arg(QString::fromUtf8(responseData));
        RLogger::error("[ROpenAiAgent] [session: %s] Unexpected HTTP status %d.\n",
                       sessionId.toUtf8().constData(), httpStatus);
        emit errorOccurred(errorString);
        return;
    }

    RLogger::info("[ROpenAiAgent] [session: %s] Response received (HTTP %d).\n",
                  sessionId.toUtf8().constData(), httpStatus);

    const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    if (responseDoc.isNull() || !responseDoc.isObject()) {
        const QString errorString = QStringLiteral("Failed to parse response JSON.");
        RLogger::error("[ROpenAiAgent] [session: %s] %s\n",
                       sessionId.toUtf8().constData(),
                       errorString.toUtf8().constData());
        emit errorOccurred(errorString);
        return;
    }

    const QJsonObject responseObj = responseDoc.object();

    if (responseObj.contains(QStringLiteral("error"))) {
        const QJsonObject apiError = responseObj[QStringLiteral("error")].toObject();
        const QString errorMessage = apiError[QStringLiteral("message")].toString();
        RLogger::error("[ROpenAiAgent] [session: %s] API error: %s\n",
                       sessionId.toUtf8().constData(),
                       errorMessage.toUtf8().constData());
        emit errorOccurred(errorMessage);
        return;
    }

    // OpenAI response: { "choices": [ { "message": { "role": "assistant", "content": "..." } } ] }
    const QJsonArray choices = responseObj[QStringLiteral("choices")].toArray();
    if (choices.isEmpty()) {
        const QString errorString = QStringLiteral("Response contains no choices.");
        RLogger::error("[ROpenAiAgent] [session: %s] %s\n",
                       sessionId.toUtf8().constData(),
                       errorString.toUtf8().constData());
        emit errorOccurred(errorString);
        return;
    }

    const QJsonObject messageObj =
        choices.first().toObject()[QStringLiteral("message")].toObject();
    const QString responseText = messageObj[QStringLiteral("content")].toString();

    if (responseText.isEmpty()) {
        const QString errorString = QStringLiteral("Response message content is empty.");
        RLogger::error("[ROpenAiAgent] [session: %s] %s\n",
                       sessionId.toUtf8().constData(),
                       errorString.toUtf8().constData());
        emit errorOccurred(errorString);
        return;
    }

    // Commit this turn to the conversation history
    QJsonObject userEntry;
    userEntry[QStringLiteral("role")]    = QStringLiteral("user");
    userEntry[QStringLiteral("content")] = pendingMessage;
    conversationHistory.append(userEntry);

    QJsonObject assistantEntry;
    assistantEntry[QStringLiteral("role")]    = QStringLiteral("assistant");
    assistantEntry[QStringLiteral("content")] = responseText;
    conversationHistory.append(assistantEntry);

    RLogger::info("[ROpenAiAgent] [session: %s] Turn %d complete. History size: %d messages.\n",
                  sessionId.toUtf8().constData(),
                  static_cast<int>(conversationHistory.size() / 2),
                  static_cast<int>(conversationHistory.size()));

    emit responseReceived(responseText);
}
