#include "rai_agent.h"

#include <QHttpHeaders>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>

#include <rcl_http_client.h>
#include <rcl_http_client_settings.h>
#include <rcl_http_message.h>
#include <rcl_http_proxy_settings.h>
#include <rbl_logger.h>

RAgent::RAgent(const RAgentSettings &settings, QObject *parent)
    : QObject{parent}
    , settings{settings}
    , httpClient{nullptr}
    , remainingRetries{0}
    , requestInProgress{false}
    , cancelled{false}
    , hasPendingSettings{false}
    , maxHistoryTurns{20}
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
    if (requestInProgress)
    {
        pendingSettings = settings;
        hasPendingSettings = true;
        cancelled = true;
        httpClient->abort();
        return;
    }

    this->settings = settings;
    delete httpClient;
    httpClient = buildHttpClient();
    startNewSession();
}

void RAgent::setSystemPrompt(const QString &systemPrompt)
{
    this->settings.setSystemPrompt(systemPrompt);
}

QString RAgent::getSessionId() const
{
    return sessionId;
}

void RAgent::startNewSession()
{
    sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    conversationHistory = QJsonArray();
    previousResponseId.clear();
    RLogger::info("[RAgent] New session started: %s\n",
                  sessionId.toUtf8().constData());
}

void RAgent::cancelRequest()
{
    if (!requestInProgress) return;
    cancelled = true;
    httpClient->abort();
}

void RAgent::chat(const QString &message, const QString &attachedFileId, const QString &attachedFileMimeType)
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

    if (!attachedFileId.isEmpty() && this->settings.getType() != RAgentSettings::Type::Anthropic)
    {
        RLogger::warning("[RAgent] File attachments are only supported for the Anthropic agent type; ignoring attachment.\n");
    }

    pendingMessage = message;
    pendingAttachedFileId = (this->settings.getType() == RAgentSettings::Type::Anthropic) ? attachedFileId : QString();
    pendingAttachedFileMimeType = attachedFileMimeType;
    remainingRetries = this->settings.getRetryCount();
    QTimer::singleShot(0, this, [this]() {
        sendRequest(pendingMessage);
    });
}

void RAgent::sendRequest(const QString &message)
{
    QJsonObject body;
    body[QStringLiteral("model")] = this->settings.getModel();

    if (this->settings.getType() == RAgentSettings::Type::OpenAI)
    {
        body[QStringLiteral("input")]             = message;
        body[QStringLiteral("max_output_tokens")] = this->settings.getMaxTokens();
        if (!this->settings.getSystemPrompt().isEmpty())
        {
            body[QStringLiteral("instructions")] = this->settings.getSystemPrompt();
        }
        if (!previousResponseId.isEmpty())
        {
            body[QStringLiteral("previous_response_id")] = previousResponseId;
        }
    }
    else
    {
        QJsonArray messages;

        if (this->settings.getType() == RAgentSettings::Type::OpenAILegacy)
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
        userMessage[QStringLiteral("role")] = QStringLiteral("user");
        if (this->settings.getType() == RAgentSettings::Type::Anthropic && !pendingAttachedFileId.isEmpty())
        {
            // Reference a file uploaded via the Anthropic Files API. The file
            // block is placed before the text block as recommended by the API.
            QJsonObject fileSource;
            fileSource[QStringLiteral("type")]    = QStringLiteral("file");
            fileSource[QStringLiteral("file_id")] = pendingAttachedFileId;

            QJsonObject fileBlock;
            fileBlock[QStringLiteral("type")] = pendingAttachedFileMimeType.startsWith(QStringLiteral("image/"))
                                                    ? QStringLiteral("image")
                                                    : QStringLiteral("document");
            fileBlock[QStringLiteral("source")] = fileSource;

            QJsonObject textBlock;
            textBlock[QStringLiteral("type")] = QStringLiteral("text");
            textBlock[QStringLiteral("text")] = message;

            userMessage[QStringLiteral("content")] = QJsonArray{fileBlock,textBlock};
        }
        else
        {
            userMessage[QStringLiteral("content")] = message;
        }
        messages.append(userMessage);

        body[QStringLiteral("messages")] = messages;
        if (this->settings.getType() == RAgentSettings::Type::Anthropic)
        {
            body[QStringLiteral("max_tokens")]  = this->settings.getMaxTokens();
            body[QStringLiteral("temperature")] = this->settings.getTemperature();

            if (!this->settings.getSystemPrompt().isEmpty())
            {
                QJsonObject textBlock;
                textBlock[QStringLiteral("type")] = QStringLiteral("text");
                textBlock[QStringLiteral("text")] = this->settings.getSystemPrompt();
                QJsonObject cacheControl;
                cacheControl[QStringLiteral("type")] = QStringLiteral("ephemeral");
                textBlock[QStringLiteral("cache_control")] = cacheControl;
                body[QStringLiteral("system")] = QJsonArray{textBlock};
            }
        }
    }

    const QByteArray requestBody = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QHttpHeaders headers;
    headers.append(QHttpHeaders::WellKnownHeader::ContentType, QByteArrayLiteral("application/json"));
    if (this->settings.getType() == RAgentSettings::Type::OpenAILegacy ||
        this->settings.getType() == RAgentSettings::Type::OpenAI)
    {
        headers.append("Authorization", QByteArrayLiteral("Bearer ") + this->settings.getApiKey().toUtf8());
    }
    else if (this->settings.getType() == RAgentSettings::Type::Anthropic)
    {
        const bool isNativeAnthropicUrl = this->settings.getApiUrl().contains(QStringLiteral("anthropic.com"));
        if (isNativeAnthropicUrl)
        {
            headers.append("x-api-key", this->settings.getApiKey().toUtf8());
            headers.append("anthropic-version", QByteArrayLiteral("2023-06-01"));
        }
        else
        {
            headers.append("Authorization", QByteArrayLiteral("Bearer ") + this->settings.getApiKey().toUtf8());
        }
        if (!pendingAttachedFileId.isEmpty())
        {
            // Referencing a Files API file requires the beta opt-in header.
            headers.append("anthropic-beta", QByteArrayLiteral("files-api-2025-04-14"));
        }
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

    if (cancelled)
    {
        cancelled = false;
        if (hasPendingSettings)
        {
            hasPendingSettings = false;
            this->settings = pendingSettings;
            delete httpClient;
            httpClient = buildHttpClient();
            startNewSession();
        }
        RLogger::info("[RAgent] [session: %s] Request cancelled.\n", sessionId.toUtf8().constData());
        emit requestCancelled();
        return;
    }

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

    if (responseObj.contains(QStringLiteral("error")) && !responseObj[QStringLiteral("error")].isNull())
    {
        const QJsonValue errorVal = responseObj[QStringLiteral("error")];
        QString err;
        if (errorVal.isObject())
        {
            err = errorVal.toObject()[QStringLiteral("message")].toString();
            if (err.isEmpty())
            {
                err = QString::fromUtf8(QJsonDocument(errorVal.toObject()).toJson(QJsonDocument::Compact));
            }
        }
        else if (errorVal.isString())
        {
            err = errorVal.toString();
        }
        if (err.isEmpty())
        {
            err = QString::fromUtf8(QJsonDocument(responseObj).toJson(QJsonDocument::Compact));
        }
        RLogger::error("[RAgent] [session: %s] API error: %s\n",
                       sessionId.toUtf8().constData(), err.toUtf8().constData());
        emit errorOccurred(err);
        return;
    }

    QString responseText;
    if (this->settings.getType() == RAgentSettings::Type::OpenAILegacy)
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
    else if (this->settings.getType() == RAgentSettings::Type::OpenAI)
    {
        const QJsonArray output = responseObj[QStringLiteral("output")].toArray();
        if (output.isEmpty())
        {
            const QString err = QStringLiteral("Response contains no output.");
            RLogger::error("[RAgent] [session: %s] %s\n",
                           sessionId.toUtf8().constData(), err.toUtf8().constData());
            emit errorOccurred(err);
            return;
        }

        for (const QJsonValue &item : output)
        {
            const QJsonObject itemObj = item.toObject();
            if (itemObj[QStringLiteral("type")].toString() == QStringLiteral("message"))
            {
                const QJsonArray content = itemObj[QStringLiteral("content")].toArray();
                for (const QJsonValue &block : content)
                {
                    const QJsonObject blockObj = block.toObject();
                    if (blockObj[QStringLiteral("type")].toString() == QStringLiteral("output_text"))
                    {
                        responseText += blockObj[QStringLiteral("text")].toString();
                    }
                }
            }
        }

        if (responseText.isEmpty())
        {
            const QString err = QStringLiteral("Response output text is empty.");
            RLogger::error("[RAgent] [session: %s] %s\n",
                           sessionId.toUtf8().constData(), err.toUtf8().constData());
            emit errorOccurred(err);
            return;
        }

        previousResponseId = responseObj[QStringLiteral("id")].toString();
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

    if (this->settings.getType() != RAgentSettings::Type::OpenAI)
    {
        // History keeps the plain text only; an attached file is referenced
        // solely in the turn it was sent with.
        QJsonObject userEntry;
        userEntry[QStringLiteral("role")]    = QStringLiteral("user");
        userEntry[QStringLiteral("content")] = pendingMessage;
        conversationHistory.append(userEntry);

        QJsonObject assistantEntry;
        assistantEntry[QStringLiteral("role")]    = QStringLiteral("assistant");
        assistantEntry[QStringLiteral("content")] = responseText;
        conversationHistory.append(assistantEntry);

        // Trim history to the configured window (each turn = 2 entries)
        while (conversationHistory.size() > maxHistoryTurns * 2)
            conversationHistory.removeFirst();
    }

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
