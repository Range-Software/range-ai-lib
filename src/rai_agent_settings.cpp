#include "rai_agent_settings.h"

RAgentSettings::RAgentSettings(Type type)
    : type{type}
    , temperature{1.0}
    , maxTokens{4096}
    , topP{1.0}
    , timeout{30000}
    , proxyPort{0}
    , retryCount{3}
{
    switch (type)
    {
        case Type::Anthropic:
            this->apiUrl = QStringLiteral("https://api.anthropic.com/v1/messages");
            this->model = QStringLiteral("claude-sonnet-4-20250514");
            break;
        case Type::OpenAI:
            this->apiUrl = QStringLiteral("https://api.openai.com/v1/chat/completions");
            this->model = QStringLiteral("gpt-4-0613");
            break;
        default:
            break;
    }
}

RAgentSettings::RAgentSettings(const RAgentSettings &other)
    : type{other.type}
    , apiKey{other.apiKey}
    , apiUrl{other.apiUrl}
    , model{other.model}
    , systemPrompt{other.systemPrompt}
    , temperature{other.temperature}
    , maxTokens{other.maxTokens}
    , topP{other.topP}
    , timeout{other.timeout}
    , proxyHost{other.proxyHost}
    , proxyPort{other.proxyPort}
    , proxyUser{other.proxyUser}
    , proxyPassword{other.proxyPassword}
    , retryCount{other.retryCount}
{
}

RAgentSettings &RAgentSettings::operator=(const RAgentSettings &other)
{
    if (this != &other) {
        this->type = other.type;
        this->apiKey = other.apiKey;
        this->apiUrl = other.apiUrl;
        this->model = other.model;
        this->systemPrompt = other.systemPrompt;
        this->temperature = other.temperature;
        this->maxTokens = other.maxTokens;
        this->topP = other.topP;
        this->timeout = other.timeout;
        this->proxyHost = other.proxyHost;
        this->proxyPort = other.proxyPort;
        this->proxyUser = other.proxyUser;
        this->proxyPassword = other.proxyPassword;
        this->retryCount = other.retryCount;
    }
    return *this;
}

RAgentSettings::~RAgentSettings()
{
}

RAgentSettings::Type RAgentSettings::getType() const
{
    return this->type;
}

void RAgentSettings::setType(Type type)
{
    this->type = type;
}

QString RAgentSettings::getApiKey() const
{
    return this->apiKey;
}

void RAgentSettings::setApiKey(const QString &apiKey)
{
    this->apiKey = apiKey;
}

QString RAgentSettings::getApiUrl() const
{
    return this->apiUrl;
}

void RAgentSettings::setApiUrl(const QString &apiUrl)
{
    this->apiUrl = apiUrl;
}

QString RAgentSettings::getModel() const
{
    return this->model;
}

void RAgentSettings::setModel(const QString &model)
{
    this->model = model;
}

QString RAgentSettings::getSystemPrompt() const
{
    return this->systemPrompt;
}

void RAgentSettings::setSystemPrompt(const QString &systemPrompt)
{
    this->systemPrompt = systemPrompt;
}

double RAgentSettings::getTemperature() const
{
    return this->temperature;
}

void RAgentSettings::setTemperature(double temperature)
{
    this->temperature = temperature;
}

int RAgentSettings::getMaxTokens() const
{
    return this->maxTokens;
}

void RAgentSettings::setMaxTokens(int maxTokens)
{
    this->maxTokens = maxTokens;
}

double RAgentSettings::getTopP() const
{
    return this->topP;
}

void RAgentSettings::setTopP(double topP)
{
    this->topP = topP;
}

int RAgentSettings::getTimeout() const
{
    return this->timeout;
}

void RAgentSettings::setTimeout(int timeout)
{
    this->timeout = timeout;
}

QString RAgentSettings::getProxyHost() const
{
    return this->proxyHost;
}

void RAgentSettings::setProxyHost(const QString &proxyHost)
{
    this->proxyHost = proxyHost;
}

int RAgentSettings::getProxyPort() const
{
    return this->proxyPort;
}

void RAgentSettings::setProxyPort(int proxyPort)
{
    this->proxyPort = proxyPort;
}

QString RAgentSettings::getProxyUser() const
{
    return this->proxyUser;
}

void RAgentSettings::setProxyUser(const QString &proxyUser)
{
    this->proxyUser = proxyUser;
}

QString RAgentSettings::getProxyPassword() const
{
    return this->proxyPassword;
}

void RAgentSettings::setProxyPassword(const QString &proxyPassword)
{
    this->proxyPassword = proxyPassword;
}

int RAgentSettings::getRetryCount() const
{
    return this->retryCount;
}

void RAgentSettings::setRetryCount(int retryCount)
{
    this->retryCount = retryCount;
}

RAgentSettings RAgentSettings::fromJson(const QJsonObject &json)
{
    RAgentSettings settings;

    if (const QJsonValue &v = json["type"]; v.isString())
    {
        settings.type = RAgentSettings::stringToType(v.toString());
    }
    if (const QJsonValue &v = json["apiKey"]; v.isString())
    {
        settings.apiKey = v.toString();
    }
    if (const QJsonValue &v = json["apiUrl"]; v.isString())
    {
        settings.apiUrl = v.toString();
    }
    if (const QJsonValue &v = json["model"]; v.isString())
    {
        settings.model = v.toString();
    }
    if (const QJsonValue &v = json["systemPrompt"]; v.isString())
    {
        settings.systemPrompt = v.toString();
    }
    if (const QJsonValue &v = json["temperature"]; v.isDouble())
    {
        settings.temperature = v.toDouble();
    }
    if (const QJsonValue &v = json["maxTokens"]; v.isDouble())
    {
        settings.maxTokens = v.toInt();
    }
    if (const QJsonValue &v = json["topP"]; v.isDouble())
    {
        settings.topP = v.toDouble();
    }
    if (const QJsonValue &v = json["timeout"]; v.isDouble())
    {
        settings.timeout = v.toInt();
    }
    if (const QJsonValue &v = json["retryCount"]; v.isDouble())
    {
        settings.retryCount = v.toInt();
    }

    return settings;
}

QJsonObject RAgentSettings::toJson() const
{
    QJsonObject json;

    json["type"] = RAgentSettings::typeToString(this->type);
    json["apiKey"] = this->apiKey;
    json["apiUrl"] = this->apiUrl;
    json["model"] = this->model;
    json["systemPrompt"] = this->systemPrompt;
    json["temperature"] = this->temperature;
    json["maxTokens"] = this->maxTokens;
    json["topP"] = this->topP;
    json["timeout"] = this->timeout;
    json["retryCount"] = this->retryCount;

    return json;
}

QString RAgentSettings::typeToString(Type type)
{
    switch (type)
    {
        case RAgentSettings::Type::Anthropic:
            return "Anthropic";
        case RAgentSettings::Type::OpenAI:
            return "OpenAI";
        case RAgentSettings::Type::None:
        default:
            return "None";
    }
}

RAgentSettings::Type RAgentSettings::stringToType(const QString &typeString)
{
    for (int i=0;i<int(RAgentSettings::Type::nTypes);i++)
    {
        RAgentSettings::Type type = RAgentSettings::Type(i);
        if (typeString == RAgentSettings::typeToString(type))
        {
            return type;
        }
    }
    return RAgentSettings::Type::None;
}
