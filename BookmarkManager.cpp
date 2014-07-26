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

    if (!query.first())
        return Error(retrieveError, query.lastError());

    bdata.BID      = query.record().value("BID"     ).toLongLong();
    bdata.Name     = query.record().value("Name"    ).toString();
    bdata.URL      = query.record().value("URL"     ).toString();
    bdata.Desc     = query.record().value("Desc"    ).toString();
    bdata.DefFile  = query.record().value("DefFile" ).toLongLong();
    bdata.Rating   = query.record().value("Rating"  ).toInt();

    return true;
}

bool BookmarkManager::AddOrEditBookmark(long long BID, const BookmarkManager::BookmarkData& bdata)
{
    QString updateError = (BID == -1
                           ? "Could not add bookmark information to database."
                           : "Could not edit bookmark information.");

    QString querystr;
    if (BID == -1)
    {
        querystr =
                "INSERT INTO Bookmark (Name, URL, Desc, DefFile, Rating) "
                "VALUES (?, ?, ?, ?, ?)";
    }
    else
    {
        querystr =
                "UPDATE Bookmark "
                "SET Name = ?, URL = ?, Desc = ?, DefFile = ?, Rating = ? "
                "WHERE BID = ?";
    }

    QSqlQuery query(db);
    query.prepare(querystr);

    query.addBindValue(bdata.Name);
    query.addBindValue(bdata.URL);
    query.addBindValue(bdata.Desc);
    query.addBindValue(bdata.DefFile);
    query.addBindValue(bdata.Rating);

    if (BID != -1)
        query.addBindValue(BID);

    if (!query.exec())
        return Error(updateError, query.lastError());

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
               "  Desc TEXT, DefFile INTEGER, Rating INTEGER )");
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
    bidx.DefFile  = model.record().indexOf("DefFile" );
    bidx.Rating   = model.record().indexOf("Rating"  );

    model.setHeaderData(bidx.BID     , Qt::Horizontal, "BID"         );
    model.setHeaderData(bidx.Name    , Qt::Horizontal, "Name"        );
    model.setHeaderData(bidx.URL     , Qt::Horizontal, "URL"         );
    model.setHeaderData(bidx.Desc    , Qt::Horizontal, "Description" );
    model.setHeaderData(bidx.DefFile , Qt::Horizontal, "Default File");
    model.setHeaderData(bidx.Rating  , Qt::Horizontal, "Rating"      );
}
