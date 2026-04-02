#ifndef RAI_AGENT_SETTINGS_MANAGER_H
#define RAI_AGENT_SETTINGS_MANAGER_H

#include "rai_agent_settings.h"

#include <QObject>
#include <QList>

class RAgentSettingsManager : public QObject
{

    Q_OBJECT


    protected:

        QMap<QString,RAgentSettings> settingsMap;

    public:

        //! Constructor.
        explicit RAgentSettingsManager(QObject *parent = nullptr);

        //! Read from file.
        void read(const QString &fileName);

        //! Write to file.
        void write(const QString &fileName) const;

        //! Return true if settings with given name is present.
        bool containsName(const QString &name) const;

        //! Return list of names.
        QStringList getNames() const;

        //! Find settings.
        RAgentSettings findSettings(const QString &name, const RAgentSettings &defaultSettings = RAgentSettings()) const;

        //! Insert settings.
        void insertSettings(const QString &name, const RAgentSettings &settingsMap);

        //! Rename settings.
        void renameSettings(const QString &oldName, const QString &newName);

        //! Remove settings. Return number of items that were removed.
        qsizetype removeSettings(const QString &name);

    private:

        //! Create settings manager object from Json.
        void fromJson(const QJsonObject &json);

        //! Create Json from settings manager object.
        QJsonObject toJson() const;

    signals:

        //! Settings added.
        void settingsAdded(const QString &name);

        //! Settings renamed.
        void settingsRenamed(const QString &oldName, const QString &newName);

        //! Settings changed.
        void settingsChanged(const QString &name, const RAgentSettings &settings);

        //! Settings removed.
        void settingsRemoved(const QString &name);

};

#endif // RAI_AGENT_SETTINGS_MANAGER_H
