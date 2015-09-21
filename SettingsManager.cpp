#include "SettingsManager.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

SettingsManager::SettingsManager(QWidget* dialogParent, Config* conf)
    : ISubManager(dialogParent, conf)
{
}

QString SettingsManager::GetSetting(const QString& name, const QString& defaultValue)
{
    if (m_settings.contains(name))
        return m_settings[name];
    else
        return defaultValue;
}

bool SettingsManager::HaveSetting(const QString& name)
{
    return m_settings.contains(name);
}

bool SettingsManager::SetSetting(const QString& name, QString value)
{
    QString updateError = "Error while updating settings in the database.";

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM Settings WHERE Name = ?");
    query.addBindValue(name);

    if (!query.exec() || !query.first())
        return Error(updateError + "\nError while checking existence.", query.lastError());

    int count = query.value(0).toInt();

    QString querystr;
    if (count == 0)
        querystr = "INSERT INTO Settings (Value, Name) VALUES (?, ?)";
    else
        querystr = "UPDATE Settings SET Value = ? WHERE Name = ?";

    query.prepare(querystr);
    query.addBindValue(value);
    query.addBindValue(name);

    if (!query.exec())
        return Error(updateError, query.lastError());

    //Don't forget to update the internal hash too.
    m_settings[name] = value;

    return true;
}

bool SettingsManager::DeleteSetting(const QString& name)
{
    QString deleteError = "Error while removing settings in the database.";

    QSqlQuery query(db);
    query.prepare("DELETE FROM Settings WHERE Name = ?");
    query.addBindValue(name);

    if (!query.exec())
        return Error(deleteError, query.lastError());

    //Don't forget to update the internal hash too.
    m_settings.remove(name);

    return true;
}

void SettingsManager::CreateTables()
{
    QSqlQuery query(db);
    query.exec("CREATE TABLE Settings "
               "( SetID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, Value TEXT )");
}

void SettingsManager::PopulateModelsAndInternalTables()
{
    //SettingsManager does not have any models; we populate our internal hash.

    QSqlQuery query(db);
    query.prepare("SELECT * FROM Settings");

    if (!query.exec())
    {
        Error("Error in getting settings from the database.", query.lastError());
        return;
    }

    while (query.next())
    {
        const QSqlRecord record = query.record();
        QString name = record.value("Name").toString();
        QString value = record.value("Value").toString();
        m_settings[name] = value;
    }
}
