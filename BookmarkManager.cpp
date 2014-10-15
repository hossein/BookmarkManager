#include "BookmarkManager.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

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
