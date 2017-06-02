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

    bool HaveSetting(const QString& name);

    //Since GetSetting reads from internal hash, it doesn't return errors.
    QString GetSetting(const QString& name, const QString& defaultValue);
    bool GetSetting(const QString& name, bool defaultValue);
    int GetSetting(const QString& name, int defaultValue);
    qint64 GetSetting(const QString& name, qint64 defaultValue);

    //SetSetting does DB access.
    bool SetSetting(const QString& name, const QString& value);
    bool SetSetting(const QString& name, bool value);
    bool SetSetting(const QString& name, int value);
    bool SetSetting(const QString& name, qint64 value);

    //DeleteSetting does DB access.
    bool DeleteSetting(const QString& name);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModelsAndInternalTables();
};
