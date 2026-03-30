#include "rai_open_ai_agent_settings.h"

ROpenAiAgentSettings::ROpenAiAgentSettings()
    : m_apiUrl(QStringLiteral("https://api.openai.com/v1/chat/completions"))
    , m_model(QStringLiteral("gpt-4o"))
    , m_temperature(1.0)
    , m_maxTokens(4096)
    , m_topP(1.0)
    , m_timeout(30000)
    , m_proxyPort(0)
    , m_retryCount(3)
{
}

ROpenAiAgentSettings::ROpenAiAgentSettings(const ROpenAiAgentSettings &other)
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

ROpenAiAgentSettings &ROpenAiAgentSettings::operator=(const ROpenAiAgentSettings &other)
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

ROpenAiAgentSettings::~ROpenAiAgentSettings()
{
}

QString ROpenAiAgentSettings::apiKey() const
{
    return m_apiKey;
}

void ROpenAiAgentSettings::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

QString ROpenAiAgentSettings::apiUrl() const
{
    return m_apiUrl;
}

void ROpenAiAgentSettings::setApiUrl(const QString &apiUrl)
{
    m_apiUrl = apiUrl;
}

QString ROpenAiAgentSettings::model() const
{
    return m_model;
}

void ROpenAiAgentSettings::setModel(const QString &model)
{
    m_model = model;
}

QString ROpenAiAgentSettings::systemPrompt() const
{
    return m_systemPrompt;
}

void ROpenAiAgentSettings::setSystemPrompt(const QString &systemPrompt)
{
    m_systemPrompt = systemPrompt;
}

double ROpenAiAgentSettings::temperature() const
{
    return m_temperature;
}

void ROpenAiAgentSettings::setTemperature(double temperature)
{
    m_temperature = temperature;
}

int ROpenAiAgentSettings::maxTokens() const
{
    return m_maxTokens;
}

void ROpenAiAgentSettings::setMaxTokens(int maxTokens)
{
    m_maxTokens = maxTokens;
}

double ROpenAiAgentSettings::topP() const
{
    return m_topP;
}

void ROpenAiAgentSettings::setTopP(double topP)
{
    m_topP = topP;
}

int ROpenAiAgentSettings::timeout() const
{
    return m_timeout;
}

void ROpenAiAgentSettings::setTimeout(int timeout)
{
    m_timeout = timeout;
}

QString ROpenAiAgentSettings::proxyHost() const
{
    return m_proxyHost;
}

void ROpenAiAgentSettings::setProxyHost(const QString &proxyHost)
{
    m_proxyHost = proxyHost;
}

int ROpenAiAgentSettings::proxyPort() const
{
    return m_proxyPort;
}

void ROpenAiAgentSettings::setProxyPort(int proxyPort)
{
    m_proxyPort = proxyPort;
}

QString ROpenAiAgentSettings::proxyUser() const
{
    return m_proxyUser;
}

void ROpenAiAgentSettings::setProxyUser(const QString &proxyUser)
{
    m_proxyUser = proxyUser;
}

QString ROpenAiAgentSettings::proxyPassword() const
{
    return m_proxyPassword;
}

void ROpenAiAgentSettings::setProxyPassword(const QString &proxyPassword)
{
    m_proxyPassword = proxyPassword;
}

int ROpenAiAgentSettings::retryCount() const
{
    return m_retryCount;
}

void ROpenAiAgentSettings::setRetryCount(int retryCount)
{
    m_retryCount = retryCount;
}
