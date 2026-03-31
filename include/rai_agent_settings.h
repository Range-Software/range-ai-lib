#ifndef RAI_AGENT_SETTINGS_H
#define RAI_AGENT_SETTINGS_H

#include <QString>

class RAgentSettings
{

    private:

        // API Settings
        QString m_apiKey;
        QString m_apiUrl;
        QString m_model;

        // Request Settings
        QString m_systemPrompt;
        double m_temperature;
        int m_maxTokens;
        double m_topP;

        // Network Settings
        int m_timeout;
        QString m_proxyHost;
        int m_proxyPort;
        QString m_proxyUser;
        QString m_proxyPassword;
        int m_retryCount;

    public:

        RAgentSettings();
        RAgentSettings(const RAgentSettings &other);
        RAgentSettings &operator=(const RAgentSettings &other);
        ~RAgentSettings();

        // API Settings
        QString apiKey() const;
        void setApiKey(const QString &apiKey);

        QString apiUrl() const;
        void setApiUrl(const QString &apiUrl);

        QString model() const;
        void setModel(const QString &model);

        // Request Settings
        QString systemPrompt() const;
        void setSystemPrompt(const QString &systemPrompt);

        double temperature() const;
        void setTemperature(double temperature);

        int maxTokens() const;
        void setMaxTokens(int maxTokens);

        double topP() const;
        void setTopP(double topP);

        // Network Settings
        int timeout() const;
        void setTimeout(int timeout);

        QString proxyHost() const;
        void setProxyHost(const QString &proxyHost);

        int proxyPort() const;
        void setProxyPort(int proxyPort);

        QString proxyUser() const;
        void setProxyUser(const QString &proxyUser);

        QString proxyPassword() const;
        void setProxyPassword(const QString &proxyPassword);

        int retryCount() const;
        void setRetryCount(int retryCount);

        static RAgentSettings defaultAnthropicSettings();

        static RAgentSettings defaultOpenAiSettings();

};

#endif // RAI_AGENT_SETTINGS_H
