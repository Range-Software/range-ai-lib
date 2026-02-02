#include "rai_claude_agent_settings.h"

RClaudeAgentSettings::RClaudeAgentSettings()
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

RClaudeAgentSettings::RClaudeAgentSettings(const RClaudeAgentSettings &other)
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

RClaudeAgentSettings &RClaudeAgentSettings::operator=(const RClaudeAgentSettings &other)
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

RClaudeAgentSettings::~RClaudeAgentSettings()
{
}

QString RClaudeAgentSettings::apiKey() const
{
    return m_apiKey;
}

void RClaudeAgentSettings::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

QString RClaudeAgentSettings::apiUrl() const
{
    return m_apiUrl;
}

void RClaudeAgentSettings::setApiUrl(const QString &apiUrl)
{
    m_apiUrl = apiUrl;
}

QString RClaudeAgentSettings::model() const
{
    return m_model;
}

void RClaudeAgentSettings::setModel(const QString &model)
{
    m_model = model;
}

QString RClaudeAgentSettings::systemPrompt() const
{
    return m_systemPrompt;
}

void RClaudeAgentSettings::setSystemPrompt(const QString &systemPrompt)
{
    m_systemPrompt = systemPrompt;
}

double RClaudeAgentSettings::temperature() const
{
    return m_temperature;
}

void RClaudeAgentSettings::setTemperature(double temperature)
{
    m_temperature = temperature;
}

int RClaudeAgentSettings::maxTokens() const
{
    return m_maxTokens;
}

void RClaudeAgentSettings::setMaxTokens(int maxTokens)
{
    m_maxTokens = maxTokens;
}

double RClaudeAgentSettings::topP() const
{
    return m_topP;
}

void RClaudeAgentSettings::setTopP(double topP)
{
    m_topP = topP;
}

int RClaudeAgentSettings::timeout() const
{
    return m_timeout;
}

void RClaudeAgentSettings::setTimeout(int timeout)
{
    m_timeout = timeout;
}

QString RClaudeAgentSettings::proxyHost() const
{
    return m_proxyHost;
}

void RClaudeAgentSettings::setProxyHost(const QString &proxyHost)
{
    m_proxyHost = proxyHost;
}

int RClaudeAgentSettings::proxyPort() const
{
    return m_proxyPort;
}

void RClaudeAgentSettings::setProxyPort(int proxyPort)
{
    m_proxyPort = proxyPort;
}

QString RClaudeAgentSettings::proxyUser() const
{
    return m_proxyUser;
}

void RClaudeAgentSettings::setProxyUser(const QString &proxyUser)
{
    m_proxyUser = proxyUser;
}

QString RClaudeAgentSettings::proxyPassword() const
{
    return m_proxyPassword;
}

void RClaudeAgentSettings::setProxyPassword(const QString &proxyPassword)
{
    m_proxyPassword = proxyPassword;
}

int RClaudeAgentSettings::retryCount() const
{
    return m_retryCount;
}

void RClaudeAgentSettings::setRetryCount(int retryCount)
{
    m_retryCount = retryCount;
}
