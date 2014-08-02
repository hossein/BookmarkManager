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

    bdata.BID      = query.record().value("BID"     ).toLongLong();
    bdata.Name     = query.record().value("Name"    ).toString();
    bdata.URL      = query.record().value("URL"     ).toString();
    bdata.Desc     = query.record().value("Desc"    ).toString();
    bdata.DefBFID  = query.record().value("DefBFID" ).toLongLong();
    bdata.Rating   = query.record().value("Rating"  ).toInt();
    bdata.AddDate  = query.record().value("AddDate" ).toLongLong();

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

void BookmarkManager::CreateTables()
{
    QSqlQuery query(db);

    query.exec("CREATE TABLE Bookmark"
               "( BID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, URL TEXT, "
               "  Desc TEXT, DefBFID INTEGER, Rating INTEGER, AddDate INTEGER )");

    query.exec("CREATE TABLE BookmarkTrash"
               "( BID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, URL TEXT, "
               "  Desc TEXT, DefBFID INTEGER, Rating INTEGER, AddDate INTEGER )");
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
