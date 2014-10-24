#include "BookmarkManager.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

#include "Util.h"

BookmarkManager::BookmarkManager(QWidget* dialogParent, Config* conf)
    : ISubManager(dialogParent, conf)
{
}

bool BookmarkManager::RetrieveBookmark(long long BID, BookmarkManager::BookmarkData& bdata)
{
    QString retrieveError = "Could not get bookmark information from database.";
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Bookmark WHERE BID = ?");
    query.addBindValue(BID);

    if (!query.exec())
        return Error(retrieveError, query.lastError());

    if (!query.first()) //Simply no results where returned.
        return Error(retrieveError + "\nThe selected bookmark was not found.");

    const QSqlRecord record = query.record();
    bdata.BID      = record.value("BID"     ).toLongLong();
    bdata.Name     = record.value("Name"    ).toString();
    bdata.URL      = record.value("URL"     ).toString();
    bdata.Desc     = record.value("Desc"    ).toString();
    bdata.DefBFID  = record.value("DefBFID" ).toLongLong();
    bdata.Rating   = record.value("Rating"  ).toInt();
    bdata.AddDate  = record.value("AddDate" ).toLongLong();

    return true;
}

bool BookmarkManager::AddOrEditBookmark(long long& BID, BookmarkManager::BookmarkData& bdata)
{
    QString updateError = (BID == -1
                           ? "Could not add bookmark information to database."
                           : "Could not edit bookmark information.");

    QString querystr;
    if (BID == -1)
    {
        querystr =
                "INSERT INTO Bookmark (Name, URL, Desc, DefBFID, Rating, AddDate) "
                "VALUES (?, ?, ?, ?, ?, ?)";
    }
    else
    {
        querystr =
                "UPDATE Bookmark "
                "SET Name = ?, URL = ?, Desc = ?, DefBFID = ?, Rating = ? "
                "WHERE BID = ?";
    }

    QSqlQuery query(db);
    query.prepare(querystr);

    query.addBindValue(bdata.Name);
    query.addBindValue(bdata.URL);
    query.addBindValue(bdata.Desc);
    query.addBindValue(bdata.DefBFID);
    query.addBindValue(bdata.Rating);

    if (BID == -1) //We ignore bdata.AddDate. We manage it ourselves ONLY upon Adding a bookmark.
        query.addBindValue(QDateTime::currentMSecsSinceEpoch()); //Add
    else
        query.addBindValue(BID); //Edit

    if (!query.exec())
        return Error(updateError, query.lastError());

    if (BID == -1)
    {
        long long addedBID = query.lastInsertId().toLongLong();
        BID = addedBID;
        bdata.BID = addedBID;
    }

    return true;
}

bool BookmarkManager::SetBookmarkDefBFID(long long BID, long long BFID)
{
    QString setDefBFIDError =
            "Could not alter attached files information for the bookmark in the database.";

    QSqlQuery query(db);
    query.prepare("UPDATE Bookmark SET DefBFID = ? WHERE BID = ?");
    query.addBindValue(BFID);
    query.addBindValue(BID);

    if (!query.exec())
        return Error(setDefBFIDError, query.lastError());

    return true;
}

bool BookmarkManager::DeleteBookmark(long long BID)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM Bookmark WHERE BID = ?");
    query.addBindValue(BID);

    if (!query.exec())
        return Error("Could not delete bookmark.", query.lastError());

    return true;
}

bool BookmarkManager::RetrieveLinkedBookmarks(long long BID, QList<long long>& linkedBIDs)
{
    QString retrieveError = "Could not retrieve linked bookmark information from database.";
    QSqlQuery query(db);
    query.prepare("SELECT BID1 AS BID FROM BookmarkLink WHERE BID2 = ? "
                  "UNION "
                  "SELECT BID2 AS BID FROM BookmarkLink WHERE BID1 = ? ");
    query.addBindValue(BID);
    query.addBindValue(BID);

    if (!query.exec())
        return Error(retrieveError, query.lastError());

    linkedBIDs.clear(); //Do it for caller.
    while (query.next())
        linkedBIDs.append(query.value(0).toLongLong());

    return true;
}

bool BookmarkManager::UpdateLinkedBookmarks(long long BID, const QList<long long>& originalLinkedBIDs,
                                            const QList<long long>& editedLinkedBIDs)
{
    //Remaining original BIDs that were not in the edited BIDs must be removed.
    QList<long long> removeLinkedBIDs = originalLinkedBIDs;
    //New edited BIDs that were not in original BIDs are the new links that we have to add.
    QList<long long> addLinkedBIDs = editedLinkedBIDs;

    UtilT::ListDifference<long long>(removeLinkedBIDs, addLinkedBIDs);

    foreach (long long removeBID, removeLinkedBIDs)
        if (!RemoveBookmarksLink(BID, removeBID))
            return false;

    foreach (long long addBID, addLinkedBIDs)
        if (!LinkBookmarksTogether(BID, addBID))
            return false;

    return true;
}

bool BookmarkManager::LinkBookmarksTogether(long long BID1, long long BID2)
{
    if (BID1 == BID2 || BID1 == -1 || BID2 == -1) //A simple check before doing anything.
        return true;

    QString retrieveError = "Could not get information for linking bookmarks from database.";
    QSqlQuery query(db);
    query.prepare("SELECT * FROM BookmarkLink WHERE "
                  "(BID1 = ? AND BID2 = ?) OR (BID1 = ? AND BID2 = ?)");
    query.addBindValue(BID1);
    query.addBindValue(BID2);
    query.addBindValue(BID2);
    query.addBindValue(BID1);

    if (!query.exec())
        return Error(retrieveError, query.lastError());

    if (query.first()) //Already linked.
        return true;

    QString linkError = "Could not set bookmark linking information in database.";
    query.prepare("INSERT INTO BookmarkLink(BID1, BID2) VALUES (?, ?)");
    query.addBindValue(BID1);
    query.addBindValue(BID2);

    if (!query.exec())
        return Error(linkError, query.lastError());

    return true;
}

bool BookmarkManager::RemoveBookmarksLink(long long BID1, long long BID2)
{
    QString deleteError = "Could not alter information for linking bookmarks in database.";
    QSqlQuery query(db);
    query.prepare("DELETE FROM BookmarkLink WHERE "
                  "(BID1 = ? AND BID2 = ?) OR (BID1 = ? AND BID2 = ?)");
    query.addBindValue(BID1);
    query.addBindValue(BID2);
    query.addBindValue(BID2);
    query.addBindValue(BID1);

    if (!query.exec())
        return Error(deleteError, query.lastError());

    return true;
}

bool BookmarkManager::RetrieveBookmarkExtraInfos(long long BID, QList<BookmarkManager::BookmarkExtraInfoData>& extraInfos)
{
    QString retrieveError = "Could not get bookmarks' extra information from database.";
    QSqlQuery query(db);
    query.prepare("SELECT * FROM BookmarkExtraInfo WHERE BID = ?");
    query.addBindValue(BID);

    if (!query.exec())
        return Error(retrieveError, query.lastError());

    bool indexesCalculated = false;
    int beiidx_BEIID, beiidx_Name, beiidx_Type, beiidx_Value;

    extraInfos.clear(); //Do it for caller
    while (query.next())
    {
        if (!indexesCalculated)
        {
            const QSqlRecord& record = query.record();
            beiidx_BEIID = record.indexOf("BEIID");
            beiidx_Name  = record.indexOf("Name" );
            beiidx_Type  = record.indexOf("Type" );
            beiidx_Value = record.indexOf("Value");
            indexesCalculated = true;
        }

        BookmarkExtraInfoData exInfo;
        exInfo.BEIID = query.value(beiidx_BEIID).toLongLong();
        exInfo.Name  = query.value(beiidx_Name).toString();
        exInfo.Type  = static_cast<BookmarkExtraInfoData::DataType>(query.value(beiidx_Type).toInt());
        exInfo.Value = query.value(beiidx_Value).toString();

        extraInfos.append(exInfo);
    }

    return true;
}

static bool BookmarkExtraInfoNameEquals(const BookmarkManager::BookmarkExtraInfoData& exInfo1,
                                        const BookmarkManager::BookmarkExtraInfoData& exInfo2)
{
    return 0 == QString::compare(exInfo1.Name, exInfo2.Name, Qt::CaseSensitive);
}

bool BookmarkManager::UpdateBookmarkExtraInfos(long long BID, const QList<BookmarkManager::BookmarkExtraInfoData>& extraInfos,
                                               const QList<BookmarkManager::BookmarkExtraInfoData>& originalExtraInfos)
{
    //Remaining extraInfo Names that were not in the edited extraInfos must be removed.
    QList<BookmarkExtraInfoData> removeExtraInfos = originalExtraInfos;
    //New edited extraInfo Names that were not in original extraInfos are the new extraInfos that we have to add.
    QList<BookmarkExtraInfoData> addExtraInfos = extraInfos;

    //Calculate the difference
    UtilT::ListDifference<BookmarkExtraInfoData>(removeExtraInfos, addExtraInfos, BookmarkExtraInfoNameEquals);

    QString deleteError = "Could not alter bookmark extra information in database.";
    QString addError = "Could not add bookmark extra information to database.";
    QSqlQuery query(db);

    //Remove the un-useds
    int removeCount = 0;
    QString BEIIDsToRemove;
    foreach (const BookmarkExtraInfoData& removeExtraInfo, removeExtraInfos)
    {
        if (removeCount > 0)
            BEIIDsToRemove.append(",");
        BEIIDsToRemove.append(QString::number(removeExtraInfo.BEIID));
        removeCount++;
    }

    if (removeCount > 0)
    {
        query.prepare(QString("DELETE FROM BookmarkExtraInfo WHERE BEIID IN (%1)").arg(BEIIDsToRemove));
        query.addBindValue(BEIIDsToRemove);

        if (!query.exec())
            return Error(deleteError, query.lastError());
    }

    //Add new extra infos
    //  We could use multiple-insert notation to make this more outside-of-transaction-friendly.
    //  But then again, for SQLite < 3.7.11 there is a limit of 500 compound select statements, etc
    //  which makes situation complicated.
    foreach (const BookmarkExtraInfoData& addExtraInfo, addExtraInfos)
    {
        query.prepare("INSERT INTO BookmarkExtraInfo(BID, Name, Type, Value) VALUE (?, ?, ?, ?)");
        query.addBindValue(BID); //Don't use addExtraInfo's one. More error-proof.
        query.addBindValue(addExtraInfo.Name);
        query.addBindValue(static_cast<int>(addExtraInfo.Type));
        query.addBindValue(addExtraInfo.Value);

        if (!query.exec())
            return Error(addError, query.lastError());
    }

    return true;
}

void BookmarkManager::CreateTables()
{
    QSqlQuery query(db);

    query.exec("CREATE TABLE Bookmark"
               "( BID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, URL TEXT, "
               "  Desc TEXT, DefBFID INTEGER, Rating INTEGER, AddDate INTEGER )");

    query.exec("CREATE TABLE BookmarkTrash"
               "( BID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, URL TEXT, "
               "  Desc TEXT, DefBFID INTEGER, Rating INTEGER, AddDate INTEGER )");

    query.exec("CREATE Table BookmarkLink"
               "( BLID INTEGER PRIMARY KEY AUTOINCREMENT, BID1 INTEGER, BID2 INTEGER )");

    query.exec("CREATE Table BookmarkExtraInfo"
               "( BEIID INTEGER PRIMARY KEY AUTOINCREMENT, BID INTEGER, Name TEXT, Type TEXT, Value TEXT )");
}

void BookmarkManager::PopulateModels()
{
    model.setQuery("SELECT * FROM Bookmark", db);

    while (model.canFetchMore())
        model.fetchMore();

    bidx.BID      = model.record().indexOf("BID"     );
    bidx.Name     = model.record().indexOf("Name"    );
    bidx.URL      = model.record().indexOf("URL"     );
    bidx.Desc     = model.record().indexOf("Desc"    );
    bidx.DefBFID  = model.record().indexOf("DefBFID" );
    bidx.Rating   = model.record().indexOf("Rating"  );
    bidx.AddDate  = model.record().indexOf("AddDate" );

    model.setHeaderData(bidx.BID     , Qt::Horizontal, "BID"         );
    model.setHeaderData(bidx.Name    , Qt::Horizontal, "Name"        );
    model.setHeaderData(bidx.URL     , Qt::Horizontal, "URL"         );
    model.setHeaderData(bidx.Desc    , Qt::Horizontal, "Description" );
    model.setHeaderData(bidx.DefBFID , Qt::Horizontal, "Default File");
    model.setHeaderData(bidx.Rating  , Qt::Horizontal, "Rating"      );
    model.setHeaderData(bidx.AddDate , Qt::Horizontal, "Date Added"  );
}
