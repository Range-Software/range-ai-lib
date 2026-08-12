#ifndef RAI_AGENT_H
#define RAI_AGENT_H

#include <QJsonArray>
#include <QObject>

#include "rai_agent_settings.h"

class RHttpClient;

class RAgent : public QObject
{
    Q_OBJECT

    private:

        //! Agent settings.
        RAgentSettings settings;

        //! HTTP client.
        RHttpClient *httpClient;

        //! Confirmed conversation turns sent to the API on every request.
        QJsonArray conversationHistory;

        //! Unique identifier for the current conversation session.
        QString sessionId;

        //! Message pending retry or in-flight.
        QString pendingMessage;

        //! Anthropic file id attached to the pending message (Files API).
        QString pendingAttachedFileId;

        //! Mime type of the attached file (decides between document and image block).
        QString pendingAttachedFileMimeType;

        //! Remaining retry attempts for current request.
        int remainingRetries;

        //! Whether a request is currently in progress.
        bool requestInProgress;

        //! Whether the current request has been cancelled.
        bool cancelled;

        //! Settings update deferred because a request was in progress when setSettings was called.
        RAgentSettings pendingSettings;
        bool hasPendingSettings;

        //! Maximum number of conversation turns kept in history (older turns are dropped).
        int maxHistoryTurns;

        //! Last response ID from the OpenAI Responses API; used as previous_response_id in subsequent requests.
        QString previousResponseId;

    public:

        //! Constructor. Starts a new session automatically.
        explicit RAgent(const RAgentSettings &settings, QObject *parent = nullptr);

        //! Destructor.
        ~RAgent() override;

        //! Return current agent settings.
        const RAgentSettings &getSettings() const;

        //! Return current agent settings.
        RAgentSettings &getSettings();

        //! Return the current session identifier.
        QString getSessionId() const;

        //! Clear conversation history and assign a new session identifier.
        void startNewSession();

    public slots:

        //! Set new agent settings. Resets conversation history and rebuilds the HTTP client.
        void setSettings(const RAgentSettings &settings);

        //! Update the system prompt only, without resetting history or rebuilding the HTTP client.
        void setSystemPrompt(const QString &systemPrompt);

        //! Send a user message and asynchronously receive a response.
        //! Previous turns in the session are included as context.
        //! An optional file already uploaded to the Anthropic Files API can be
        //! attached to the message (Anthropic agent type only); the mime type
        //! decides whether it is referenced as a document or an image block.
        void chat(const QString &message, const QString &attachedFileId = QString(), const QString &attachedFileMimeType = QString());

        //! Cancel the current in-progress request. No-op if no request is in progress.
        void cancelRequest();

    private:

        //! Build and dispatch the HTTP POST request.
        void sendRequest(const QString &message);

        //! Build and return an RHttpClient configured from current settings.
        RHttpClient *buildHttpClient();

    signals:

        //! Emitted when the assistant response has been received.
        void responseReceived(const QString &response);

        //! Emitted when the request fails and all retries are exhausted.
        void errorOccurred(const QString &error);

        //! Emitted when the request was cancelled via cancelRequest().
        void requestCancelled();

};

#endif // RAI_AGENT_H
