#include "rai_agent_settings.h"

RAgentSettings::RAgentSettings()
    : m_apiUrl(QStringLiteral("https://api.anthropic.com/v1/messages"))
    , m_model(QStringLiteral("claude-sonnet-4-20250514"))
    , m_temperature(1.0)
    , m_maxTokens(4096)
    , m_topP(1.0)
    , m_timeout(30000)
    , m_proxyPort(0)
    , m_retryCount(3)
{
}

RAgentSettings::RAgentSettings(const RAgentSettings &other)
    : m_apiKey(other.m_apiKey)
    , m_apiUrl(other.m_apiUrl)
    , m_model(other.m_model)
    , m_systemPrompt(other.m_systemPrompt)
    , m_temperature(other.m_temperature)
    , m_maxTokens(other.m_maxTokens)
    , m_topP(other.m_topP)
    , m_timeout(other.m_timeout)
    , m_proxyHost(other.m_proxyHost)
    , m_proxyPort(other.m_proxyPort)
    , m_proxyUser(other.m_proxyUser)
    , m_proxyPassword(other.m_proxyPassword)
    , m_retryCount(other.m_retryCount)
{
}

RAgentSettings &RAgentSettings::operator=(const RAgentSettings &other)
{
    if (this != &other) {
        m_apiKey = other.m_apiKey;
        m_apiUrl = other.m_apiUrl;
        m_model = other.m_model;
        m_systemPrompt = other.m_systemPrompt;
        m_temperature = other.m_temperature;
        m_maxTokens = other.m_maxTokens;
        m_topP = other.m_topP;
        m_timeout = other.m_timeout;
        m_proxyHost = other.m_proxyHost;
        m_proxyPort = other.m_proxyPort;
        m_proxyUser = other.m_proxyUser;
        m_proxyPassword = other.m_proxyPassword;
        m_retryCount = other.m_retryCount;
    }
    return *this;
}

RAgentSettings::~RAgentSettings()
{
}

QString RAgentSettings::apiKey() const
{
    return m_apiKey;
}

void RAgentSettings::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

QString RAgentSettings::apiUrl() const
{
    return m_apiUrl;
}

void RAgentSettings::setApiUrl(const QString &apiUrl)
{
    m_apiUrl = apiUrl;
}

QString RAgentSettings::model() const
{
    return m_model;
}

void RAgentSettings::setModel(const QString &model)
{
    m_model = model;
}

QString RAgentSettings::systemPrompt() const
{
    return m_systemPrompt;
}

void RAgentSettings::setSystemPrompt(const QString &systemPrompt)
{
    m_systemPrompt = systemPrompt;
}

double RAgentSettings::temperature() const
{
    return m_temperature;
}

void RAgentSettings::setTemperature(double temperature)
{
    m_temperature = temperature;
}

int RAgentSettings::maxTokens() const
{
    return m_maxTokens;
}

void RAgentSettings::setMaxTokens(int maxTokens)
{
    m_maxTokens = maxTokens;
}

double RAgentSettings::topP() const
{
    return m_topP;
}

void RAgentSettings::setTopP(double topP)
{
    m_topP = topP;
}

int RAgentSettings::timeout() const
{
    return m_timeout;
}

void RAgentSettings::setTimeout(int timeout)
{
    m_timeout = timeout;
}

QString RAgentSettings::proxyHost() const
{
    return m_proxyHost;
}

void RAgentSettings::setProxyHost(const QString &proxyHost)
{
    m_proxyHost = proxyHost;
}

int RAgentSettings::proxyPort() const
{
    return m_proxyPort;
}

void RAgentSettings::setProxyPort(int proxyPort)
{
    m_proxyPort = proxyPort;
}

QString RAgentSettings::proxyUser() const
{
    return m_proxyUser;
}

void RAgentSettings::setProxyUser(const QString &proxyUser)
{
    m_proxyUser = proxyUser;
}

QString RAgentSettings::proxyPassword() const
{
    return m_proxyPassword;
}

void RAgentSettings::setProxyPassword(const QString &proxyPassword)
{
    m_proxyPassword = proxyPassword;
}

int RAgentSettings::retryCount() const
{
    return m_retryCount;
}

void RAgentSettings::setRetryCount(int retryCount)
{
    m_retryCount = retryCount;
}

RAgentSettings defaultAnthropicSettings()
{
    RAgentSettings settings;
    settings.setApiUrl(QStringLiteral("https://api.anthropic.com/v1/messages"));
    settings.setModel(QStringLiteral("claude-sonnet-4-20250514"));
    return settings;
}

RAgentSettings defaultOpenAiSettings()
{
    RAgentSettings settings;
    settings.setApiUrl(QStringLiteral("https://api.openai.com/v1/chat/completions"));
    settings.setModel(QStringLiteral("gpt-4-0613"));
    return settings;
}
