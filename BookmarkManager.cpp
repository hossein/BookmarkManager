#include "BookmarkManager.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>
#include <QtSql/QSqlTableModel>

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

bool BookmarkManager::RemoveBookmark(long long BID)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM Bookmark WHERE BID = ?");
    query.addBindValue(BID);

    if (!query.exec())
        return Error("Could not remove bookmark.", query.lastError());

    return true;
}

bool BookmarkManager::InsertBookmarkIntoTrash(
        const QString& Name, const QString& URL, const QString& Description, const QString& Tags,
        const QString& AttachedFIDs, const long long DefFID, const int Rating, long long AddDate)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO BookmarkTrash(Name, URL, Desc, AttachedFIDs, DefFID, Rating, Tags, "
                  "                          DeleteDate, AddDate) VALUES (?,?,?,?,?,?,?,?,?)");
    query.addBindValue(Name);
    query.addBindValue(URL);
    query.addBindValue(Description);
    query.addBindValue(AttachedFIDs);
    query.addBindValue(DefFID);
    query.addBindValue(Rating);
    query.addBindValue(Tags);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(AddDate);

    if (!query.exec())
        return Error("Could not trash the bookmark.", query.lastError());

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

bool BookmarkManager::RetrieveBookmarkExtraInfosModel(long long BID, QSqlTableModel& extraInfosModel)
{
    QString retrieveError = "Could not get bookmark extra information from database.";

    //Since the query is already executed, we think no errors happened in here.
    extraInfosModel.setTable("BookmarkExtraInfo");
    extraInfosModel.setFilter("BID = " + QString::number(BID));
    if (!extraInfosModel.select())
        return Error(retrieveError, extraInfosModel.lastError());

    while (extraInfosModel.canFetchMore())
        extraInfosModel.fetchMore();

    SetBookmarkExtraInfoIndexes(extraInfosModel.record());
    extraInfosModel.setHeaderData(beiidx.BEIID, Qt::Horizontal, "BEIID");
    extraInfosModel.setHeaderData(beiidx.BID  , Qt::Horizontal, "BID"  );
    extraInfosModel.setHeaderData(beiidx.Name , Qt::Horizontal, "Name" );
    extraInfosModel.setHeaderData(beiidx.Type , Qt::Horizontal, "Type" );
    extraInfosModel.setHeaderData(beiidx.Value, Qt::Horizontal, "Value");

    //Important: Prevent changes to the database while the user is editing.
    //  Database changes must ONLY be made during a TRANSACTION and with appropriate PREPROCESSING
    //  done in the appropriate function for updating the extra infos using the model.
    extraInfosModel.setEditStrategy(QSqlTableModel::OnManualSubmit);

    return true;
}

bool BookmarkManager::UpdateBookmarkExtraInfos(long long BID, QSqlTableModel& extraInfosModel)
{
    //It is IMPORTANT that we set BIDs before submitting. This way we eliminate the need to connect to
    //  e.g rowAboutToBeInserted and assign the BID value there.
    //The same applies for the BEIID field's 'Generated' attribute.
    //  Note that if client uses the utility functions provided in this class to insert the bookmarks,
    //  BID and the BEIID generated are already set. But it's okay.

    //Set BIDs and set BEIIDs generated to false.
    const int rowCount = extraInfosModel.rowCount();
    for (int row = 0; row < rowCount; row++)
    {
        extraInfosModel.record(row).setGenerated(beiidx.BEIID, false);
        extraInfosModel.setData(extraInfosModel.index(row, beiidx.BID), BID);
    }

    //Submit and check for errors
    bool success = extraInfosModel.submitAll();

    if (!success)
        return Error("Could not update bookmark extra information in database.", extraInfosModel.lastError());

    return true;
}

void BookmarkManager::InsertBookmarkExtraInfoIntoModel(
        QSqlTableModel& extraInfosModel, long long BID,
        const QString& Name, BookmarkManager::BookmarkExtraInfoData::DataType Type, const QString& Value)
{
    QSqlRecord record = extraInfosModel.record(); //Must do it! Otherwise setting values doesn't work.
    record.setGenerated(beiidx.BEIID, false); //Dunno should set this or not. Better set it.
    record.setValue(beiidx.BID, BID);
    record.setValue(beiidx.Name, Name);
    record.setValue(beiidx.Type, static_cast<int>(Type));
    record.setValue(beiidx.Value, Value);

    //[Return value not checked. Assume success.]
    extraInfosModel.insertRecord(-1, record);
}

void BookmarkManager::RemoveBookmarkExtraInfoFromModel(QSqlTableModel& extraInfosModel, const QModelIndex& index)
{
    //[Return value not checked. Assume success.]
    extraInfosModel.removeRow(index.row());
}

bool BookmarkManager::RetrieveBookmarkExtraInfos(long long BID, QList<BookmarkManager::BookmarkExtraInfoData>& extraInfos)
{
    QString retrieveError = "Could not get bookmark extra information from database.";
    QSqlQuery query(db);
    query.prepare("SELECT * FROM BookmarkExtraInfo WHERE BID = ?");
    query.addBindValue(BID);

    if (!query.exec())
        return Error(retrieveError, query.lastError());

    SetBookmarkExtraInfoIndexes(query.record()); //TODO: Does this populate correctly when query doesn't have values? Same code and placement is used for the MAIN THING: bookmarks too!
    bool indexesCalculated = false;
    int beiidx_BEIID, beiidx_BID, beiidx_Name, beiidx_Type, beiidx_Value;

    extraInfos.clear(); //Do it for caller
    while (query.next())
    {
        if (!indexesCalculated)
        {
            const QSqlRecord& record = query.record();
            beiidx_BEIID = record.indexOf("BEIID");
            beiidx_BID   = record.indexOf("BID"  );
            beiidx_Name  = record.indexOf("Name" );
            beiidx_Type  = record.indexOf("Type" );
            beiidx_Value = record.indexOf("Value");
            indexesCalculated = true;
        }

        BookmarkExtraInfoData exInfo;
        exInfo.BEIID = query.value(beiidx_BEIID).toLongLong();
        exInfo.BID   = query.value(beiidx_BID).toLongLong();
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
    //TODO: Must compare BEIID, not Name to allow same-name properties.
    return 0 == QString::compare(exInfo1.Name, exInfo2.Name, Qt::CaseSensitive);
}

bool BookmarkManager::UpdateBookmarkExtraInfos(long long BID, const QList<BookmarkManager::BookmarkExtraInfoData>& originalExtraInfos,
                                               const QList<BookmarkManager::BookmarkExtraInfoData>& extraInfos)
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

bool BookmarkManager::RetrieveAllFullURLs(QHash<long long, QString>& bookmarkURLs)
{
    QString retrieveError = "Could not get bookmarks information from database.";
    QSqlQuery query(db);
    query.prepare("SELECT BID, URL FROM Bookmark");

    if (!query.exec())
        return Error(retrieveError, query.lastError());

    //Do it for caller
    bookmarkURLs.clear();

    while (query.next())
        //We indexed explicitly in our select statement; indexes are constant.
        bookmarkURLs.insert(query.value(0).toLongLong(), query.value(1).toString());

    return true;
}

bool BookmarkManager::RetrieveSpecificExtraInfoForAllBookmarks(const QString& extraInfoName,
                                                               QList<BookmarkExtraInfoData>& extraInfos)
{
    QString retrieveError = "Could not get bookmarks extra information from database.";
    QSqlQuery query(db);
    query.prepare("SELECT * FROM BookmarkExtraInfo WHERE Name = ?");
    query.addBindValue(extraInfoName);

    if (!query.exec())
        return Error(retrieveError, query.lastError());

    bool indexesCalculated = false;
    int beiidx_BID, beiidx_Name, beiidx_Type, beiidx_Value;

    extraInfos.clear(); //Do it for caller
    while (query.next())
    {
        if (!indexesCalculated)
        {
            const QSqlRecord& record = query.record();
            beiidx_BID   = record.indexOf("BID"  );
            beiidx_Name  = record.indexOf("Name" );
            beiidx_Type  = record.indexOf("Type" );
            beiidx_Value = record.indexOf("Value");
            indexesCalculated = true;
        }

        BookmarkExtraInfoData exInfo;
        exInfo.BID   = query.value(beiidx_BID).toLongLong();
        exInfo.Name  = query.value(beiidx_Name).toString();
        exInfo.Type  = static_cast<BookmarkExtraInfoData::DataType>(query.value(beiidx_Type).toInt());
        exInfo.Value = query.value(beiidx_Value).toString();

        extraInfos.append(exInfo);
    }

    return true;
}

void BookmarkManager::SetBookmarkExtraInfoIndexes(const QSqlRecord& record)
{
    beiidx.BEIID = record.indexOf("BEIID");
    beiidx.BID   = record.indexOf("BID"  );
    beiidx.Name  = record.indexOf("Name" );
    beiidx.Type  = record.indexOf("Type" );
    beiidx.Value = record.indexOf("Value");
}

void BookmarkManager::CreateTables()
{
    QSqlQuery query(db);

    query.exec("CREATE TABLE Bookmark"
               "( BID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, URL TEXT, "
               "  Desc TEXT, DefBFID INTEGER, Rating INTEGER, AddDate INTEGER )");

    query.exec("CREATE TABLE BookmarkTrash"
               "( BID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, URL TEXT, "
               "  Desc TEXT, AttachedFIDs TEXT, DefFID INTEGER, Rating INTEGER, "
               "  Tags TEXT, DeleteDate INTEGER, AddDate INTEGER )");

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
