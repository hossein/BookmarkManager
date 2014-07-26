#include "TagManager.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

TagManager::TagManager(QWidget* dialogParent, Config* conf)
    : ISubManager(dialogParent, conf)
{
}

bool TagManager::RetrieveBookmarkTags(long long BID, QStringList& tagsList)
{
    QString retrieveError = "Could not get tag information for bookmark from database.";
    QSqlQuery query(db);
    query.prepare("SELECT * FROM BookmarkTag NATURAL JOIN Tag WHERE BID = ?");
    query.addBindValue(BID);

    if (!query.exec())
        return Error(retrieveError, query.lastError());

    int indexOfTagName = query.record().indexOf("TagName");
    while (query.next())
        tagsList.append(query.value(indexOfTagName).toString());

    return true;
}

void TagManager::CreateTables()
{
    QSqlQuery query(db);

    query.exec("CREATE TABLE Tag( TID INTEGER PRIMARY KEY AUTOINCREMENT, TagName TEXT )");

    query.exec("CREATE TABLE BookmarkTag"
               "( BTID INTEGER PRIMARY KEY AUTOINCREMENT, BID INTEGER, TID INTEGER )");
}

void TagManager::PopulateModels()
{
    model.setQuery("SELECT * FROM Tag", db);

    while (model.canFetchMore())
        model.fetchMore();

    tidx.TID     = model.record().indexOf("TID"    );
    tidx.TagName = model.record().indexOf("TagName");

    model.setHeaderData(tidx.TID    , Qt::Horizontal, "TID"    );
    model.setHeaderData(tidx.TagName, Qt::Horizontal, "TagName");
}
