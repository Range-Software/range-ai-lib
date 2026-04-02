#include "rai_agent_settings_manager.h"

#include <rbl_logger.h>
#include <rbl_error.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

RAgentSettingsManager::RAgentSettingsManager(QObject *parent)
    : QObject{parent}
{}

void RAgentSettingsManager::read(const QString &fileName)
{
    RLogger::info("Reading session file \"%s\".\n",fileName.toUtf8().constData());

    QFile inFile(fileName);

    if (!inFile.exists())
    {
        throw RError(RError::Type::InvalidFileName,R_ERROR_REF,"Session file \"%s\" does not exist.",fileName.toUtf8().constData());
    }

    if(!inFile.open(QIODevice::ReadOnly))
    {
        throw RError(RError::Type::OpenFile,R_ERROR_REF,
                     "Failed to open settings manager file \"%s\" for reading. %s.",
                     inFile.fileName().toUtf8().constData(),
                     inFile.errorString().toUtf8().constData());
    }

    QByteArray byteArray = inFile.readAll();
    RLogger::info("Successfuly read \"%ld\" bytes from \"%s\".\n",byteArray.size(),inFile.fileName().toUtf8().constData());

    this->fromJson(QJsonDocument::fromJson(byteArray).object());

    inFile.close();
}

void RAgentSettingsManager::write(const QString &fileName) const
{
    RLogger::info("Writing settings manager file \"%s\".\n",fileName.toUtf8().constData());

    QFile outFile(fileName);

    if(!outFile.open(QIODevice::WriteOnly))
    {
        throw RError(RError::Type::OpenFile,R_ERROR_REF,
                     "Failed to open settings manager file \"%s\" for writing. %s.",
                     outFile.fileName().toUtf8().constData(),
                     outFile.errorString().toUtf8().constData());
    }

    qint64 bytesOut = outFile.write(QJsonDocument(this->toJson()).toJson());

    RLogger::info("Successfuly wrote \"%ld\" bytes to \"%s\".\n",bytesOut,outFile.fileName().toUtf8().constData());

    outFile.close();
}

bool RAgentSettingsManager::containsName(const QString &name) const
{
    return this->settingsMap.contains(name);
}

QStringList RAgentSettingsManager::getNames() const
{
    return this->settingsMap.keys();
}

RAgentSettings RAgentSettingsManager::findSettings(const QString &name, const RAgentSettings &defaultSettings) const
{
    if (this->settingsMap.contains(name))
    {
        return this->settingsMap.value(name);
    }
    return defaultSettings;
}

void RAgentSettingsManager::insertSettings(const QString &name, const RAgentSettings &settings)
{
    bool changed = this->settingsMap.contains(name);
    this->settingsMap.insert(name,settings);
    if (changed)
    {
        emit this->settingsChanged(name,settings);
    }
    else
    {
        emit this->settingsAdded(name);
    }
}

void RAgentSettingsManager::renameSettings(const QString &oldName, const QString &newName)
{
    this->settingsMap.insert(newName,settingsMap.value(oldName));
    this->settingsMap.remove(oldName);
    emit this->settingsRenamed(newName,newName);
}

qsizetype RAgentSettingsManager::removeSettings(const QString &name)
{
    qsizetype n = this->settingsMap.remove(name);
    emit this->settingsRemoved(name);
    return n;
}

void RAgentSettingsManager::fromJson(const QJsonObject &json)
{
    if (const QJsonValue &v = json["settingsMap"]; v.isArray())
    {
        const QJsonArray &settingsArray = v.toArray();
        this->settingsMap.clear();
        for (const QJsonValue &item : settingsArray)
        {
            if (const QJsonValue &v = item["name"]; v.isString())
            {
                QString name = item["name"].toString();

                RAgentSettings settings;
                if (const QJsonValue &v = item["settings"]; v.isObject())
                {
                    settings = RAgentSettings::fromJson(item["settings"].toObject());
                }
                this->settingsMap.insert(name,settings);
            }
        }
    }
}

QJsonObject RAgentSettingsManager::toJson() const
{
    QJsonObject json;

    QJsonArray settingsArray;
    for (auto i = this->settingsMap.cbegin(), end = this->settingsMap.cend(); i != end; ++i)
    {
        QJsonObject jsonSettings;
        jsonSettings["name"] = i.key();
        jsonSettings["settings"] = i.value().toJson();
        settingsArray.append(jsonSettings);
    }
    json["settingsMap"] = settingsArray;

    return json;
}
