#include "rai_agent.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QUrl>
#include <QUuid>

#include "rbl_logger.h"

RAgent::RAgent(const RAgentSettings &settings, QObject *parent)
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
        RLogger::info("[RAgent] Proxy configured: %s:%d\n",
                      settings.proxyHost().toUtf8().constData(),
                      settings.proxyPort());
    }

    networkManager->setTransferTimeout(settings.timeout());
    startNewSession();
}

RAgent::~RAgent()
{
    if (networkReply) {
        networkReply->abort();
        networkReply->deleteLater();
    }
}

QString RAgent::getSessionId() const
{
    return sessionId;
}

void RAgent::startNewSession()
{
    sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    conversationHistory = QJsonArray();
    RLogger::info("[RAgent] New session started: %s\n",
                  sessionId.toUtf8().constData());
}

void RAgent::chat(const QString &message)
{
    if (message.isEmpty()) {
        RLogger::warning("[RAgent] chat() called with an empty message.\n");
        return;
    }

    if (networkReply && networkReply->isRunning()) {
        RLogger::warning("[RAgent] A request is already in progress; ignoring new chat() call.\n");
        return;
    }

    pendingMessage = message;
    remainingRetries = settings.retryCount();
    sendRequest(message);
}

void RAgent::sendRequest(const QString &message)
{
    QJsonArray messages;

    if (agentType == RagentType::OpenAI)
    {
        // Build messages array: optional system prompt + confirmed history + current user turn.
        // The OpenAI API carries the system prompt as a message with role "system".

        if (!settings.systemPrompt().isEmpty()) {
            QJsonObject systemMessage;
            systemMessage[QStringLiteral("role")]    = QStringLiteral("system");
            systemMessage[QStringLiteral("content")] = settings.systemPrompt();
            messages.append(systemMessage);
        }

        for (const QJsonValue &entry : std::as_const(conversationHistory)) {
            messages.append(entry);
        }
    }
    else if (agentType == RagentType::Anthropic)
    {
        // Build messages array: confirmed history + current user turn
        QJsonArray messages = conversationHistory;
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

    if (agentType == RagentType::Anthropic)
    {
        if (!settings.systemPrompt().isEmpty())
        {
            body[QStringLiteral("system")] = settings.systemPrompt();
        }
    }

    const QByteArray requestBody = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkRequest request{QUrl(settings.apiUrl())};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json"));
    if (agentType == RagentType::OpenAI)
    {
        request.setRawHeader(QByteArrayLiteral("Authorization"),
                             QByteArrayLiteral("Bearer ") + settings.apiKey().toUtf8());
    }
    else if (agentType == RagentType::Anthropic)
    {
        request.setRawHeader(QByteArrayLiteral("x-api-key"),         settings.apiKey().toUtf8());
        request.setRawHeader(QByteArrayLiteral("anthropic-version"),  QByteArrayLiteral("2023-06-01"));
    }

    RLogger::info("[RAgent] [session: %s] Sending request (turn %d, model: %s)\n",
                  sessionId.toUtf8().constData(),
                  static_cast<int>(conversationHistory.size() / 2 + 1),
                  settings.model().toUtf8().constData());

    networkReply = networkManager->post(request, requestBody);
    QObject::connect(networkReply, &QNetworkReply::finished, this, &RAgent::onReplyFinished);
}

void RAgent::onReplyFinished()
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
            RLogger::warning("[RAgent] [session: %s] Request failed (%s), retrying (%d attempt(s) left).\n",
                             sessionId.toUtf8().constData(),
                             errorString.toUtf8().constData(),
                             remainingRetries + 1);
            sendRequest(pendingMessage);
            return;
        }

        RLogger::error("[RAgent] [session: %s] Request failed after all retries: %s\n",
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
        RLogger::error("[RAgent] [session: %s] Unexpected HTTP status %d.\n",
                       sessionId.toUtf8().constData(), httpStatus);
        emit errorOccurred(errorString);
        return;
    }

    RLogger::info("[RAgent] [session: %s] Response received (HTTP %d).\n",
                  sessionId.toUtf8().constData(), httpStatus);

    const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    if (responseDoc.isNull() || !responseDoc.isObject()) {
        const QString errorString = QStringLiteral("Failed to parse response JSON.");
        RLogger::error("[RAgent] [session: %s] %s\n",
                       sessionId.toUtf8().constData(),
                       errorString.toUtf8().constData());
        emit errorOccurred(errorString);
        return;
    }

    const QJsonObject responseObj = responseDoc.object();

    if (responseObj.contains(QStringLiteral("error"))) {
        const QJsonObject apiError = responseObj[QStringLiteral("error")].toObject();
        const QString errorMessage = apiError[QStringLiteral("message")].toString();
        RLogger::error("[RAgent] [session: %s] API error: %s\n",
                       sessionId.toUtf8().constData(),
                       errorMessage.toUtf8().constData());
        emit errorOccurred(errorMessage);
        return;
    }

    if (agentType == RagentType::OpenAI)
    {
        // OpenAI response: { "choices": [ { "message": { "role": "assistant", "content": "..." } } ] }
        const QJsonArray choices = responseObj[QStringLiteral("choices")].toArray();
        if (choices.isEmpty()) {
            const QString errorString = QStringLiteral("Response contains no choices.");
            RLogger::error("[RAgent] [session: %s] %s\n",
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
            RLogger::error("[RAgent] [session: %s] %s\n",
                           sessionId.toUtf8().constData(),
                           errorString.toUtf8().constData());
            emit errorOccurred(errorString);
            return;
        }
    }
    else if (agentType == RagentType::Anthropic)
    {
        const QJsonArray content = responseObj[QStringLiteral("content")].toArray();
        if (content.isEmpty()) {
            const QString errorString = QStringLiteral("Response content is empty.");
            RLogger::error("[RAgent] [session: %s] %s\n",
                           sessionId.toUtf8().constData(),
                           errorString.toUtf8().constData());
            emit errorOccurred(errorString);
            return;
        }

        QString responseText;
        for (const QJsonValue &block : content) {
            if (block.toObject()[QStringLiteral("type")].toString() == QStringLiteral("text")) {
                responseText += block.toObject()[QStringLiteral("text")].toString();
            }
        }
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

    RLogger::info("[RAgent] [session: %s] Turn %d complete. History size: %d messages.\n",
                  sessionId.toUtf8().constData(),
                  static_cast<int>(conversationHistory.size() / 2),
                  static_cast<int>(conversationHistory.size()));

    emit responseReceived(responseText);
}
