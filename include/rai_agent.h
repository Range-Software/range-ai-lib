#ifndef RAI_AGENT_H
#define RAI_AGENT_H

#include <QJsonArray>
#include <QObject>
#include <QNetworkReply>

#include "rai_agent_settings.h"

class QNetworkAccessManager;

class RAgent : public QObject
{
    Q_OBJECT

    public:

        enum class Type
        {
            OpenAI,
            Anthropic
        };

    private:

        //! Agent type (e.g., OpenAI, Anthropic, Custom).
        Type agentType;

        //! Agent settings.
        RAgentSettings settings;

        //! Network access manager.
        QNetworkAccessManager *networkManager;

        //! Current network reply.
        QNetworkReply *networkReply;

        //! Confirmed conversation turns sent to the API on every request.
        QJsonArray conversationHistory;

        //! Unique identifier for the current conversation session.
        QString sessionId;

        //! Message pending retry or in-flight.
        QString pendingMessage;

        //! Remaining retry attempts for current request.
        int remainingRetries;

        //! Build and dispatch the HTTP POST request.
        void sendRequest(const QString &message);

    public:

        //! Constructor. Starts a new session automatically.
        explicit RAgent(const RAgentSettings &settings, QObject *parent = nullptr);

        //! Destructor.
        ~RAgent() override;

        //! Return the current session identifier.
        QString getSessionId() const;

        //! Clear conversation history and assign a new session identifier.
        void startNewSession();

        //! Send a user message and asynchronously receive a response.
        //! Previous turns in the session are included as context.
        void chat(const QString &message);

    signals:

        //! Emitted when the assistant response has been received.
        void responseReceived(const QString &response);

        //! Emitted when the request fails and all retries are exhausted.
        void errorOccurred(const QString &error);

    private slots:

        //! Called when the network reply has finished.
        void onReplyFinished();

};

#endif // RAI_AGENT_H
