#include "TagManager.h"
#include "Util/Util.h"

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

    tagsList.clear(); //Do it for caller
    const int indexOfTagName = query.record().indexOf("TagName");
    while (query.next())
        tagsList.append(query.value(indexOfTagName).toString());

    return true;
}

bool TagManager::SetBookmarkTags(long long BID, const QStringList& tagsList,
                                 QList<long long>& associatedTIDs)
{
    QString setTagsError = "Could not alter tag information for bookmark in the database.";
    QSqlQuery query(db);

    QStringList tagsToAdd = Util::CaseInsensitiveStringListEliminateDuplicatesCopy(tagsList);
    QList<long long> tagsToRemove;

    //Empty values might come from anywhere (although we fixed Import bug that generated empty tags)
    tagsToAdd.removeAll(QString());
    tagsToAdd.removeAll(QString(""));

    // Remove those that already exist in the database from the tagsToAdd.
    query.prepare("SELECT * FROM BookmarkTag NATURAL JOIN Tag WHERE BID = ?");
    query.addBindValue(BID);
    if (!query.exec())
        return Error(setTagsError, query.lastError());

    const QSqlRecord record = query.record();
    const int indexOfBTID = record.indexOf("BTID");
    const int indexOfTagName = record.indexOf("TagName");
    while (query.next())
    {
        QString tagNameInDB = query.value(indexOfTagName).toString();
        if (tagsToAdd.contains(tagNameInDB, Qt::CaseInsensitive))
        {
            //Tag already in database. We remove it from list of our tags to add.
            Util::CaseInsensitiveStringListRemoveElement(tagsToAdd, tagNameInDB);
        }
        else
        {
            //User wants to remove the tag.
            tagsToRemove.append(query.value(indexOfBTID).toLongLong());
        }
    }

    //Remove unwanted tags from DB.
    for (int i = 0; i < tagsToRemove.count(); i++)
    {
        query.prepare("DELETE FROM BookmarkTag WHERE BTID = ?");
        query.addBindValue(tagsToRemove[i]);
        if (!query.exec())
            return Error(setTagsError, query.lastError());
    }

    //Add the new tags to DB.
    associatedTIDs.clear(); //Do it for user.
    for (int i = 0; i < tagsToAdd.count(); i++)
    {
        long long TID = MaybeCreateTagAndReturnTID(tagsToAdd[i]);
        associatedTIDs.append(TID);

        query.prepare("INSERT INTO BookmarkTag ( BID , TID ) VALUES ( ? , ? )");
        query.addBindValue(BID);
        query.addBindValue(TID);
        if (!query.exec())
            return Error(setTagsError, query.lastError());
    }

    //Note: An easier alternative to this function was to remove all tags for a bookmark then
    //  re-add them.
    //query.prepare("DELETE FROM BookmarkTag WHERE BID = ?");
    //query.addBindValue(BID);
    //if (!query.exec())
    //    return Error(setTagsError, query.lastError());

    return true;
}

bool TagManager::GetBookmarkIDsForTags(const QSet<long long>& TIDs, QSet<long long>& BIDs)
{
    /// Mostly same as BookmarkManager::RetrieveBookmarksInFolders

    QString retrieveError = "Could not get tag information for bookmarks from database.";
    QSqlQuery query(db);

    QString commaSeparatedTIDs;
    foreach (long long TID, TIDs)
        commaSeparatedTIDs += QString::number(TID) + ",";
    //Remove the last comma.
    if (commaSeparatedTIDs.length() > 0) //Empty-check
        commaSeparatedTIDs.chop(1);

    //Note: Ordering by BID or anything here is useless. We use this info to filter the data later.
    //  Also empty commaSeparatedTIDs string is fine.
    query.prepare("SELECT DISTINCT BID FROM BookmarkTag WHERE TID IN (" + commaSeparatedTIDs + ")");

    if (!query.exec())
        return Error(retrieveError, query.lastError());

    //Do it for caller
    BIDs.clear();

    //`query.record()` works even if zero bookmarks were returned (related: query.isActive|isValid).
    //  Moreover, the indexes are also correct in this case, just the record is empty.
    //if (!query.first()) //Simply no results where returned.
    //  return true;
    int indexOfBID = query.record().indexOf("BID");
    while (query.next())
        BIDs.insert(query.value(indexOfBID).toLongLong());

    return true;
}

long long TagManager::MaybeCreateTagAndReturnTID(const QString& tagName)
{
    QString setTagsError = "Could not alter tag information for bookmark in the database.";

    QSqlQuery query(db);
    query.prepare("SELECT TID FROM Tag WHERE TagName = ? COLLATE NOCASE"); //Case insensitive.
    query.addBindValue(tagName);
    if (!query.exec())
        return Error(setTagsError, query.lastError());

    if (query.first())
        return query.record().value("TID").toLongLong();

    query.prepare("INSERT INTO Tag ( TagName ) VALUES ( ? )");
    query.addBindValue(tagName);
    if (!query.exec())
        return Error(setTagsError, query.lastError());

    return query.lastInsertId().toLongLong();
}

void TagManager::CreateTables()
{
    QSqlQuery query(db);

    query.exec("CREATE TABLE Tag( TID INTEGER PRIMARY KEY AUTOINCREMENT, TagName TEXT )");

    query.exec("CREATE TABLE BookmarkTag"
               "( BTID INTEGER PRIMARY KEY AUTOINCREMENT, BID INTEGER, TID INTEGER, "
               "  FOREIGN KEY(BID) REFERENCES Bookmark(BID) ON DELETE CASCADE, "
               "  FOREIGN KEY(TID) REFERENCES Tag(TID) ON DELETE CASCADE )");
}

void TagManager::PopulateModelsAndInternalTables()
{
    model.setQuery("SELECT * FROM Tag", db);

    if (model.lastError().isValid())
    {
        Error("Error while populating tag models.", model.lastError());
        return;
    }

    while (model.canFetchMore())
        model.fetchMore();

    tidx.TID     = model.record().indexOf("TID"    );
    tidx.TagName = model.record().indexOf("TagName");

    model.setHeaderData(tidx.TID    , Qt::Horizontal, "TID"    );
    model.setHeaderData(tidx.TagName, Qt::Horizontal, "TagName");

    //For tags model, we handle the sorting very here!
    model.sort(tidx.TagName);
}
