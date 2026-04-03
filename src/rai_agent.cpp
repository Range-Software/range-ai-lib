#include "rai_agent.h"

#include <QHttpHeaders>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>

#include "rcl_http_client.h"
#include "rcl_http_client_settings.h"
#include "rcl_http_message.h"
#include "rcl_http_proxy_settings.h"
#include "rbl_logger.h"

RAgent::RAgent(const RAgentSettings &settings, QObject *parent)
    : QObject{parent}
    , settings{settings}
    , httpClient{nullptr}
    , remainingRetries{0}
    , requestInProgress{false}
{
    httpClient = buildHttpClient();
    startNewSession();
}

RAgent::~RAgent()
{
}

const RAgentSettings &RAgent::getSettings() const
{
    return this->settings;
}

RAgentSettings &RAgent::getSettings()
{
    return this->settings;
}

void RAgent::setSettings(const RAgentSettings &settings)
{
    this->settings = settings;

    delete httpClient;
    httpClient = buildHttpClient();
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
    if (message.isEmpty())
    {
        RLogger::warning("[RAgent] chat() called with an empty message.\n");
        return;
    }

    if (requestInProgress)
    {
        RLogger::warning("[RAgent] A request is already in progress; ignoring new chat() call.\n");
        return;
    }

    pendingMessage = message;
    remainingRetries = this->settings.getRetryCount();
    QTimer::singleShot(0, this, [this]() {
        sendRequest(pendingMessage);
    });
}

void RAgent::sendRequest(const QString &message)
{
    QJsonArray messages;

    if (this->settings.getType() == RAgentSettings::Type::OpenAI)
    {
        if (!this->settings.getSystemPrompt().isEmpty())
        {
            QJsonObject systemMessage;
            systemMessage[QStringLiteral("role")]    = QStringLiteral("system");
            systemMessage[QStringLiteral("content")] = this->settings.getSystemPrompt();
            messages.append(systemMessage);
        }

        for (const QJsonValue &entry : std::as_const(conversationHistory))
        {
            messages.append(entry);
        }
    }
    else if (this->settings.getType() == RAgentSettings::Type::Anthropic)
    {
        messages = conversationHistory;
    }

    QJsonObject userMessage;
    userMessage[QStringLiteral("role")]    = QStringLiteral("user");
    userMessage[QStringLiteral("content")] = message;
    messages.append(userMessage);

    QJsonObject body;
    body[QStringLiteral("model")]    = this->settings.getModel();
    body[QStringLiteral("messages")] = messages;
    if (this->settings.getType() == RAgentSettings::Type::Anthropic)
    {
        body[QStringLiteral("max_tokens")]  = this->settings.getMaxTokens();
        body[QStringLiteral("temperature")] = this->settings.getTemperature();

        if (!this->settings.getSystemPrompt().isEmpty())
        {
            body[QStringLiteral("system")] = this->settings.getSystemPrompt();
        }
    }

    const QByteArray requestBody = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QHttpHeaders headers;
    headers.append(QHttpHeaders::WellKnownHeader::ContentType, QByteArrayLiteral("application/json"));
    if (this->settings.getType() == RAgentSettings::Type::OpenAI)
    {
        headers.append("Authorization", QByteArrayLiteral("Bearer ") + this->settings.getApiKey().toUtf8());
    }
    else if (this->settings.getType() == RAgentSettings::Type::Anthropic)
    {
        headers.append("x-api-key", this->settings.getApiKey().toUtf8());
        headers.append("anthropic-version", QByteArrayLiteral("2023-06-01"));
    }

    RHttpMessage request;
    request.setMethod(QHttpServerRequest::Method::Post);
    request.setBody(requestBody);
    request.setRequestHeaders(headers);

    requestInProgress = true;

    QString errorString;
    QByteArray responseData;

    for (int attempt = 0; attempt <= remainingRetries; ++attempt)
    {
        RLogger::info("[RAgent] [session: %s] Sending request (turn %d, model: %s, attempt %d)\n",
                      sessionId.toUtf8().constData(),
                      static_cast<int>(conversationHistory.size() / 2 + 1),
                      this->settings.getModel().toUtf8().constData(),
                      attempt + 1);

        RHttpMessage reply;
        httpClient->sendRequest(request, reply);

        if (reply.getErrorType() == RError::None)
        {
            responseData = reply.getBody();
            errorString.clear();
            break;
        }

        errorString = QString::fromUtf8(reply.getBody());
        if (errorString.isEmpty())
        {
            errorString = QStringLiteral("Unknown error.");
        }

        if (attempt < remainingRetries)
        {
            RLogger::warning("[RAgent] [session: %s] Request failed (%s), retrying (%d attempt(s) left).\n",
                             sessionId.toUtf8().constData(),
                             errorString.toUtf8().constData(),
                             remainingRetries - attempt);
        }
    }

    requestInProgress = false;

    if (!errorString.isEmpty())
    {
        RLogger::error("[RAgent] [session: %s] Request failed after all retries: %s\n",
                       sessionId.toUtf8().constData(),
                       errorString.toUtf8().constData());
        emit errorOccurred(errorString);
        return;
    }

    RLogger::info("[RAgent] [session: %s] Response received.\n", sessionId.toUtf8().constData());

    const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    if (responseDoc.isNull() || !responseDoc.isObject())
    {
        const QString err = QStringLiteral("Failed to parse response JSON.");
        RLogger::error("[RAgent] [session: %s] %s\n",
                       sessionId.toUtf8().constData(), err.toUtf8().constData());
        emit errorOccurred(err);
        return;
    }

    const QJsonObject responseObj = responseDoc.object();

    if (responseObj.contains(QStringLiteral("error")))
    {
        const QJsonObject apiError = responseObj[QStringLiteral("error")].toObject();
        const QString err = apiError[QStringLiteral("message")].toString();
        RLogger::error("[RAgent] [session: %s] API error: %s\n",
                       sessionId.toUtf8().constData(), err.toUtf8().constData());
        emit errorOccurred(err);
        return;
    }

    QString responseText;
    if (this->settings.getType() == RAgentSettings::Type::OpenAI)
    {
        const QJsonArray choices = responseObj[QStringLiteral("choices")].toArray();
        if (choices.isEmpty())
        {
            const QString err = QStringLiteral("Response contains no choices.");
            RLogger::error("[RAgent] [session: %s] %s\n",
                           sessionId.toUtf8().constData(), err.toUtf8().constData());
            emit errorOccurred(err);
            return;
        }

        const QJsonObject messageObj =
            choices.first().toObject()[QStringLiteral("message")].toObject();
        responseText = messageObj[QStringLiteral("content")].toString();

        if (responseText.isEmpty())
        {
            const QString err = QStringLiteral("Response message content is empty.");
            RLogger::error("[RAgent] [session: %s] %s\n",
                           sessionId.toUtf8().constData(), err.toUtf8().constData());
            emit errorOccurred(err);
            return;
        }
    }
    else if (this->settings.getType() == RAgentSettings::Type::Anthropic)
    {
        const QJsonArray content = responseObj[QStringLiteral("content")].toArray();
        if (content.isEmpty())
        {
            const QString err = QStringLiteral("Response content is empty.");
            RLogger::error("[RAgent] [session: %s] %s\n",
                           sessionId.toUtf8().constData(), err.toUtf8().constData());
            emit errorOccurred(err);
            return;
        }

        for (const QJsonValue &block : content)
        {
            QJsonObject jObject = block.toObject();
            if (jObject[QStringLiteral("type")].toString() == QStringLiteral("text"))
            {
                responseText += jObject[QStringLiteral("text")].toString();
            }
        }
    }

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

RHttpClient *RAgent::buildHttpClient()
{
    RHttpClientSettings httpSettings;
    httpSettings.setUrl(this->settings.getApiUrl());
    httpSettings.setTimeout(static_cast<uint>(this->settings.getTimeout()));

    if (!this->settings.getProxyHost().isEmpty())
    {
        RHttpProxySettings proxy;
        proxy.setType(RHttpProxySettings::ManualProxy);
        proxy.setHost(this->settings.getProxyHost());
        proxy.setPort(static_cast<quint16>(this->settings.getProxyPort()));
        proxy.setUser(this->settings.getProxyUser());
        proxy.setPassword(this->settings.getProxyPassword());
        httpSettings.setProxySettings(proxy);
        RLogger::info("[RAgent] Proxy configured: %s:%d\n",
                      this->settings.getProxyHost().toUtf8().constData(),
                      this->settings.getProxyPort());
    }

    return new RHttpClient(RHttpClient::Type::Public, httpSettings, this);
}
