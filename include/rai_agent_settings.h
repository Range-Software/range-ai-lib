#ifndef RAI_AGENT_SETTINGS_H
#define RAI_AGENT_SETTINGS_H

#include <QString>
#include <QJsonObject>

class RAgentSettings
{

    public:

        enum class Type
        {
            None = 0,
            OpenAILegacy,
            OpenAI,
            Anthropic,
            nTypes
        };

    private:

        //! Agent type.
        RAgentSettings::Type type;

        // API Settings
        QString apiKey;
        QString apiUrl;
        QString model;

        // Request Settings
        QString systemPrompt;
        double temperature;
        int maxTokens;

        // Network Settings
        int timeout;
        QString proxyHost;
        int proxyPort;
        QString proxyUser;
        QString proxyPassword;
        int retryCount;

    public:

        RAgentSettings(RAgentSettings::Type type = RAgentSettings::Type::None);
        RAgentSettings(const RAgentSettings &other);
        RAgentSettings &operator=(const RAgentSettings &other);
        ~RAgentSettings();

        RAgentSettings::Type getType() const;
        void setType(RAgentSettings::Type type);

        // API Settings
        QString getApiKey() const;
        void setApiKey(const QString &apiKey);

        QString getApiUrl() const;
        void setApiUrl(const QString &apiUrl);

        QString getModel() const;
        void setModel(const QString &model);

        // Request Settings
        QString getSystemPrompt() const;
        void setSystemPrompt(const QString &systemPrompt);

        double getTemperature() const;
        void setTemperature(double temperature);

        int getMaxTokens() const;
        void setMaxTokens(int maxTokens);

        // Network Settings
        int getTimeout() const;
        void setTimeout(int timeout);

        QString getProxyHost() const;
        void setProxyHost(const QString &proxyHost);

        int getProxyPort() const;
        void setProxyPort(int proxyPort);

        QString getProxyUser() const;
        void setProxyUser(const QString &proxyUser);

        QString getProxyPassword() const;
        void setProxyPassword(const QString &proxyPassword);

        int getRetryCount() const;
        void setRetryCount(int retryCount);

        //! Create settings object from Json.
        static RAgentSettings fromJson(const QJsonObject &json);

        //! Create Json from settings object.
        QJsonObject toJson() const;

        //! Convert type to string.
        static QString typeToString(RAgentSettings::Type type);

        //! Convert string to type.
        static RAgentSettings::Type stringToType(const QString &typeString);

};

#endif // RAI_AGENT_SETTINGS_H
