#include "rai_anthropic_agent_settings.h"

RAnthropicAgentSettings::RAnthropicAgentSettings()
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

RAnthropicAgentSettings::RAnthropicAgentSettings(const RAnthropicAgentSettings &other)
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

RAnthropicAgentSettings &RAnthropicAgentSettings::operator=(const RAnthropicAgentSettings &other)
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

RAnthropicAgentSettings::~RAnthropicAgentSettings()
{
}

QString RAnthropicAgentSettings::apiKey() const
{
    return m_apiKey;
}

void RAnthropicAgentSettings::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

QString RAnthropicAgentSettings::apiUrl() const
{
    return m_apiUrl;
}

void RAnthropicAgentSettings::setApiUrl(const QString &apiUrl)
{
    m_apiUrl = apiUrl;
}

QString RAnthropicAgentSettings::model() const
{
    return m_model;
}

void RAnthropicAgentSettings::setModel(const QString &model)
{
    m_model = model;
}

QString RAnthropicAgentSettings::systemPrompt() const
{
    return m_systemPrompt;
}

void RAnthropicAgentSettings::setSystemPrompt(const QString &systemPrompt)
{
    m_systemPrompt = systemPrompt;
}

double RAnthropicAgentSettings::temperature() const
{
    return m_temperature;
}

void RAnthropicAgentSettings::setTemperature(double temperature)
{
    m_temperature = temperature;
}

int RAnthropicAgentSettings::maxTokens() const
{
    return m_maxTokens;
}

void RAnthropicAgentSettings::setMaxTokens(int maxTokens)
{
    m_maxTokens = maxTokens;
}

double RAnthropicAgentSettings::topP() const
{
    return m_topP;
}

void RAnthropicAgentSettings::setTopP(double topP)
{
    m_topP = topP;
}

int RAnthropicAgentSettings::timeout() const
{
    return m_timeout;
}

void RAnthropicAgentSettings::setTimeout(int timeout)
{
    m_timeout = timeout;
}

QString RAnthropicAgentSettings::proxyHost() const
{
    return m_proxyHost;
}

void RAnthropicAgentSettings::setProxyHost(const QString &proxyHost)
{
    m_proxyHost = proxyHost;
}

int RAnthropicAgentSettings::proxyPort() const
{
    return m_proxyPort;
}

void RAnthropicAgentSettings::setProxyPort(int proxyPort)
{
    m_proxyPort = proxyPort;
}

QString RAnthropicAgentSettings::proxyUser() const
{
    return m_proxyUser;
}

void RAnthropicAgentSettings::setProxyUser(const QString &proxyUser)
{
    m_proxyUser = proxyUser;
}

QString RAnthropicAgentSettings::proxyPassword() const
{
    return m_proxyPassword;
}

void RAnthropicAgentSettings::setProxyPassword(const QString &proxyPassword)
{
    m_proxyPassword = proxyPassword;
}

int RAnthropicAgentSettings::retryCount() const
{
    return m_retryCount;
}

void RAnthropicAgentSettings::setRetryCount(int retryCount)
{
    m_retryCount = retryCount;
}
