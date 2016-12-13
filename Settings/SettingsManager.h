#pragma once
#include "Database/ISubManager.h"
#include <QHash>

class DatabaseManager;

class SettingsManager : public ISubManager
{
    friend class DatabaseManager;

private:
    QHash<QString,QString> m_settings;

public:
    SettingsManager(QWidget* dialogParent, Config* conf);

    //Since GetSetting reads from internal hash, it doesn't return errors.
    QString GetSetting(const QString& name, const QString& defaultValue);
    bool GetSettingBool(const QString& name, const QString& defaultValue);
    bool HaveSetting(const QString& name);

    //SetSetting does DB access.
    bool SetSetting(const QString& name, QString value);
    bool SetSetting(const QString& name, bool value);

    //DeleteSetting does DB access.
    bool DeleteSetting(const QString& name);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModelsAndInternalTables();
};
