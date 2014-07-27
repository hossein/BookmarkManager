#pragma once
#include "ISubManager.h"
#include <QStringList>
#include <QtSql/QSqlQueryModel>

class DatabaseManager;

/// Tag management is case-insensitive. The first time you use a tag, it defines its name's
/// case style.
class TagManager : public ISubManager
{
    friend class DatabaseManager;

public:
    QSqlQueryModel model;

    struct
    {
        int TID;
        int TagName;
    } tidx;

public:
    TagManager(QWidget* dialogParent, Config* conf);

    bool RetrieveBookmarkTags(long long BID, QStringList& tagsList);
    /// It's okay if tags are duplicate.
    bool SetBookmarkTags(long long BID, const QStringList& tagsList);

private:
    long long MaybeCreateTagAndReturnTID(const QString& tagName);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
