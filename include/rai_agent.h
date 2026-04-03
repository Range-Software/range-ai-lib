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

        //! Remaining retry attempts for current request.
        int remainingRetries;

        //! Whether a request is currently in progress.
        bool requestInProgress;

    public:

        //! Constructor. Starts a new session automatically.
        explicit RAgent(const RAgentSettings &settings, QObject *parent = nullptr);

        //! Destructor.
        ~RAgent() override;

        //! Return current agent settings.
        const RAgentSettings &getSettings() const;

        //! Return current agent settings.
        RAgentSettings &getSettings();

        //! Set new agent settings.
        void setSettings(const RAgentSettings &settings);

        //! Return the current session identifier.
        QString getSessionId() const;

        //! Clear conversation history and assign a new session identifier.
        void startNewSession();

        //! Send a user message and asynchronously receive a response.
        //! Previous turns in the session are included as context.
        void chat(const QString &message);

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

};

#endif // RAI_AGENT_H
