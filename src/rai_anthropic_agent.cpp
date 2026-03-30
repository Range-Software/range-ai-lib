#include "rai_anthropic_agent.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QUrl>
#include <QUuid>

#include "rbl_logger.h"

RAnthropicAgent::RAnthropicAgent(const RAnthropicAgentSettings &settings, QObject *parent)
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
        RLogger::info("[RAnthropicAgent] Proxy configured: %s:%d\n",
                      settings.proxyHost().toUtf8().constData(),
                      settings.proxyPort());
    }

    networkManager->setTransferTimeout(settings.timeout());
    startNewSession();
}

RAnthropicAgent::~RAnthropicAgent()
{
    if (networkReply) {
        networkReply->abort();
        networkReply->deleteLater();
    }
}

QString RAnthropicAgent::getSessionId() const
{
    return sessionId;
}

void RAnthropicAgent::startNewSession()
{
    sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    conversationHistory = QJsonArray();
    RLogger::info("[RAnthropicAgent] New session started: %s\n",
                  sessionId.toUtf8().constData());
}

void RAnthropicAgent::chat(const QString &message)
{
    if (message.isEmpty()) {
        RLogger::warning("[RAnthropicAgent] chat() called with an empty message.\n");
        return;
    }

    if (networkReply && networkReply->isRunning()) {
        RLogger::warning("[RAnthropicAgent] A request is already in progress; ignoring new chat() call.\n");
        return;
    }

    pendingMessage = message;
    remainingRetries = settings.retryCount();
    sendRequest(message);
}

void RAnthropicAgent::sendRequest(const QString &message)
{
    // Build messages array: confirmed history + current user turn
    QJsonArray messages = conversationHistory;
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

    if (!settings.systemPrompt().isEmpty()) {
        body[QStringLiteral("system")] = settings.systemPrompt();
    }

    const QByteArray requestBody = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkRequest request{QUrl(settings.apiUrl())};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json"));
    request.setRawHeader(QByteArrayLiteral("x-api-key"),         settings.apiKey().toUtf8());
    request.setRawHeader(QByteArrayLiteral("anthropic-version"),  QByteArrayLiteral("2023-06-01"));

    RLogger::info("[RAnthropicAgent] [session: %s] Sending request (turn %d, model: %s)\n",
                  sessionId.toUtf8().constData(),
                  static_cast<int>(conversationHistory.size() / 2 + 1),
                  settings.model().toUtf8().constData());

    networkReply = networkManager->post(request, requestBody);
    QObject::connect(networkReply, &QNetworkReply::finished, this, &RAnthropicAgent::onReplyFinished);
}

void RAnthropicAgent::onReplyFinished()
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
            RLogger::warning("[RAnthropicAgent] [session: %s] Request failed (%s), retrying (%d attempt(s) left).\n",
                             sessionId.toUtf8().constData(),
                             errorString.toUtf8().constData(),
                             remainingRetries + 1);
            sendRequest(pendingMessage);
            return;
        }

        RLogger::error("[RAnthropicAgent] [session: %s] Request failed after all retries: %s\n",
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
        RLogger::error("[RAnthropicAgent] [session: %s] Unexpected HTTP status %d.\n",
                       sessionId.toUtf8().constData(), httpStatus);
        emit errorOccurred(errorString);
        return;
    }

    RLogger::info("[RAnthropicAgent] [session: %s] Response received (HTTP %d).\n",
                  sessionId.toUtf8().constData(), httpStatus);

    const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    if (responseDoc.isNull() || !responseDoc.isObject()) {
        const QString errorString = QStringLiteral("Failed to parse response JSON.");
        RLogger::error("[RAnthropicAgent] [session: %s] %s\n",
                       sessionId.toUtf8().constData(),
                       errorString.toUtf8().constData());
        emit errorOccurred(errorString);
        return;
    }

    const QJsonObject responseObj = responseDoc.object();

    if (responseObj.contains(QStringLiteral("error"))) {
        const QJsonObject apiError = responseObj[QStringLiteral("error")].toObject();
        const QString errorMessage = apiError[QStringLiteral("message")].toString();
        RLogger::error("[RAnthropicAgent] [session: %s] API error: %s\n",
                       sessionId.toUtf8().constData(),
                       errorMessage.toUtf8().constData());
        emit errorOccurred(errorMessage);
        return;
    }

    const QJsonArray content = responseObj[QStringLiteral("content")].toArray();
    if (content.isEmpty()) {
        const QString errorString = QStringLiteral("Response content is empty.");
        RLogger::error("[RAnthropicAgent] [session: %s] %s\n",
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

    // Commit this turn to the conversation history
    QJsonObject userEntry;
    userEntry[QStringLiteral("role")]    = QStringLiteral("user");
    userEntry[QStringLiteral("content")] = pendingMessage;
    conversationHistory.append(userEntry);

    QJsonObject assistantEntry;
    assistantEntry[QStringLiteral("role")]    = QStringLiteral("assistant");
    assistantEntry[QStringLiteral("content")] = responseText;
    conversationHistory.append(assistantEntry);

    RLogger::info("[RAnthropicAgent] [session: %s] Turn %d complete. History size: %d messages.\n",
                  sessionId.toUtf8().constData(),
                  static_cast<int>(conversationHistory.size() / 2),
                  static_cast<int>(conversationHistory.size()));

    emit responseReceived(responseText);
}
