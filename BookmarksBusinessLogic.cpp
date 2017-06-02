#include "BookmarksBusinessLogic.h"

#include "Config.h"
#include "Database/DatabaseManager.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>

BookmarksBusinessLogic::BookmarksBusinessLogic(DatabaseManager* dbm, QWidget* dialogParent)
    : dbm(dbm), dialogParent(dialogParent)
{

}

bool BookmarksBusinessLogic::RetrieveBookmarkEx(long long BID, BookmarkManager::BookmarkData& bdata,
                                                bool extraInfosModel, bool filesModel)
{
    bool success = true;

    success = dbm->bms.RetrieveBookmark(BID, bdata);
    if (!success)
        return false;

    success = dbm->bms.RetrieveLinkedBookmarks(BID, bdata.Ex_LinkedBookmarksList);
    if (!success)
        return false;

    if (extraInfosModel)
        success = dbm->bms.RetrieveBookmarkExtraInfosModel(BID, bdata.Ex_ExtraInfosModel);
    else
        success = dbm->bms.RetrieveBookmarkExtraInfos(BID, bdata.Ex_ExtraInfosList);
    if (!success)
        return false;

    success = dbm->tags.RetrieveBookmarkTags(BID, bdata.Ex_TagsList);
    if (!success)
        return false;

    if (filesModel)
        success = dbm->files.RetrieveBookmarkFilesModel(BID, bdata.Ex_FilesModel);
    else
        success = dbm->files.RetrieveBookmarkFiles(BID, bdata.Ex_FilesList);
    if (!success)
        return false;

    return success; //i.e `true`
}

void BookmarksBusinessLogic::BeginActionTransaction()
{
    //Assume success
    //We do NOT use the transactions in a nested manner. This 'linear' ('together') mode is was
    //  not only and easier to understand, it's better for error management, too.
    dbm->db.transaction();
    dbm->files.BeginFilesTransaction();
}

void BookmarksBusinessLogic::CommitActionTransaction()
{
    dbm->files.CommitFilesTransaction(); //Committing files transaction doesn't fail!
    dbm->db.commit(); //Assume doesn't fail
}

bool BookmarksBusinessLogic::RollBackActionTransaction()
{
    //Rolling back file transactions might fail, and is kinda a bad fail.
    dbm->db.rollback();

    bool rollbackResult = dbm->files.RollBackFilesTransaction();
    if (!rollbackResult)
    {
        const QString errorText = "Not all changes made to your filesystem in the intermediary "
                                  "process of adding the bookmark could not be reverted.<br/><br/>"
                                  "<b>Your filesystem may be in inconsistent state!</b>";
        qDebug() << "File Transaction Rollback Error\n" << errorText; //Will be written to log file
        QMessageBox::critical(dialogParent, "File Transaction Rollback Error", errorText);
        return false;
    }

    return true;
}

bool BookmarksBusinessLogic::AddOrEditBookmarkTrans(
        long long& editBId, BookmarkManager::BookmarkData& bdata,
        long long originalEditBId, BookmarkManager::BookmarkData& editOriginalBData,
        const QList<long long>& editedLinkedBookmarks,
        const QList<BookmarkManager::BookmarkExtraInfoData>& editedExtraInfos,
        const QStringList& tagsList, QList<long long>& associatedTIDs,
        const QList<FileManager::BookmarkFile>& editedFilesList, int defaultFileIndex)
{
    //[Similar BookmarksBusinessLogic Implementation]
    bool success;

    //IMPORTANT: In case of RollBack, do NOT return!
    //           [why-two-editbids]: In case of ADDing, MUST set editBId to its original `-1` value!
    //           Thanksfully FilesList don't need changing.
    BeginActionTransaction();
    {
        success = AddOrEditBookmark(editBId, bdata, originalEditBId, editOriginalBData,
                                    editedLinkedBookmarks, editedExtraInfos, tagsList, associatedTIDs,
                                    editedFilesList, defaultFileIndex);

        if (!success)
        {
            /// Roll back transactions and shows error if they failed, also set editBId to originalEditBId.
            /// Note: We could use a class in such a way that when we return it goes out of scope and does
            ///   these actions. It just has the advantage of typing `return false` instead of
            ///   return `DoRollBackAction(...)` though; and we prefer not to mess around in destructors.

            editBId = originalEditBId; //Affects only the Adding mode.
            RollBackActionTransaction();
            return false; //Always return false
        }
    }
    CommitActionTransaction();

    return success; //i.e `true`.
}

bool BookmarksBusinessLogic::AddOrEditBookmark(
        long long& editBId, BookmarkManager::BookmarkData& bdata,
        long long originalEditBId, BookmarkManager::BookmarkData& editOriginalBData,
        const QList<long long>& editedLinkedBookmarks,
        const QList<BookmarkManager::BookmarkExtraInfoData>& editedExtraInfos,
        const QStringList& tagsList, QList<long long>& associatedTIDs,
        const QList<FileManager::BookmarkFile>& editedFilesList, int defaultFileIndex)
{
    bool success;
    Q_UNUSED(originalEditBId);

    success = dbm->bms.AddOrEditBookmark(editBId, bdata); //For Add, the editBID will be modified!
    if (!success)
        return false;

    success = dbm->bms.UpdateLinkedBookmarks(editBId, editOriginalBData.Ex_LinkedBookmarksList,
                                             editedLinkedBookmarks);
    if (!success)
        return false;

    //Use ExtraInfos model or list based on whether model has been set or not.
    if (editOriginalBData.Ex_ExtraInfosModel.tableName().isEmpty())
    {
        success = dbm->bms.UpdateBookmarkExtraInfos(editBId, editOriginalBData.Ex_ExtraInfosList,
                                                    editedExtraInfos);
    }
    else
    {
        //DO NOT: success = editOriginalBData.Ex_ExtraInfosModel.submitAll(); Read docs
        success = dbm->bms.UpdateBookmarkExtraInfos(editBId, editOriginalBData.Ex_ExtraInfosModel);
    }
    if (!success)
        return false;

    success = dbm->tags.SetBookmarkTags(editBId, tagsList, associatedTIDs);
    if (!success)
        return false;

    //Wrong: See comments at BookmarkFolderManager::BookmarkFolderData::Ex_AbsolutePath.
    //  QString bookmarkFolderPath = dbm->bfs.bookmarkFolders[bdata.FOID].Ex_AbsolutePath;
    //Right
    QString fileArchiveName, folderHint;
    success = dbm->bfs.GetFileArchiveAndFolderHint(bdata.FOID, fileArchiveName, folderHint);
    if (!success)
        return false;

    QList<long long> updatedBFIDs;
    success = dbm->files.UpdateBookmarkFiles(editBId, folderHint, bdata.Name,
                                             editOriginalBData.Ex_FilesList, editedFilesList,
                                             updatedBFIDs, fileArchiveName,
                                             "storing bookmark files information");
    if (!success)
        return false;

    //Set the default file BFID for the bookmark. We already set defBFID to `-1` in the database
    //  according to [KeepDefaultFile-1], so we only do it if it's other than -1.
    //The following line is WRONG and can't be used! Our original `editedFilesList` hasn't changed!
    //long long editedDefBFID = DefaultBFID(editedFilesList);
    long long editedDefBFIDIndex = defaultFileIndex;
    if (editedDefBFIDIndex != -1)
    {
        long long editedDefBFID = updatedBFIDs[editedDefBFIDIndex];
        success = dbm->bms.SetBookmarkDefBFID(editBId, editedDefBFID);
    }
    if (!success)
        return false;

    return true;
}

bool BookmarksBusinessLogic::DeleteBookmarksTrans(const QList<long long>& BIDs)
{
    //[Similar BookmarksBusinessLogic Implementation]
    bool success;

    BeginActionTransaction();
    {
        foreach (long long BID, BIDs)
        {
            success = DeleteBookmark(BID);
            if (!success)
            {
                RollBackActionTransaction();
                return false; //Always return false
            }
        }
    }
    CommitActionTransaction();

    return success; //i.e `true`.
}

bool BookmarksBusinessLogic::DeleteBookmarkTrans(long long BID)
{
    return DeleteBookmarksTrans(QList<long long>() << BID);
}

bool BookmarksBusinessLogic::DeleteBookmark(long long BID)
{
    //Steps in deleting a bookmark:
    //IN A TRANSACTION:
    //  1. Convert its attached file id's to CSV string (NOT BFID).
    //  2. Get the FID of the default file instead of BFID.
    //  3. Remove the bookmark-file attachment information.
    //  4. Move its files to :trash: archive if the files aren't shared (file id's won't change).
    //  5. Convert its tag names to CSV string.
    //  6. Convert its extra info to one big chunk of text, probably a json document.
    //  7. Linked bookmarks will be forgotten, as we can't keep the BIDs.
    //  8. Convert folder ID to an absolute folder path.
    //  9. Move the information to BookmarkTrash table.

    bool success = true;
    BookmarkManager::BookmarkData bdata;

    success = RetrieveBookmarkEx(BID, bdata, false, false);
    if (!success)
        return false;

    //Convert attached file FIDs to CSV string.
    //Get the FID of the default file (we have BFID, need FID, [KeepDefaultFile-1] matters).
    long long defaultFID = -1;
    QList<long long> attachedFIDs;
    foreach (const FileManager::BookmarkFile& bf, bdata.Ex_FilesList)
    {
        attachedFIDs.append(bf.FID);
        if (bf.BFID == bdata.DefBFID)
            defaultFID = bf.FID;
    }
    QStringList attachedFIDsStrList;
    foreach (long long FID, attachedFIDs)
        attachedFIDsStrList.append(QString::number(FID));
    QString attachedFIDsStr = attachedFIDsStrList.join(",");

    //Remove BookmarkFile attachment information. This is necessary as foreign keys restrict
    //  deleting a bookmark having associated files with it.
    //Note that file transaction undo is expensive and we may want to postpone this until the
    //  very last minutes of deleting the bookmark, BUT we think sql transactions won't fail so
    //  do this expensive tasks at the beginning.
    //Send files to trash (if they're not shared).
    success = dbm->files.TrashAllBookmarkFiles(BID, "deleting bookmark files");
    if (!success)
        return false;

    //Convert tag names to csv.
    //Foreign keys cascades will later delete the tags.
    QString tagNames = bdata.Ex_TagsList.join(",");

    //Store linked bookmarks' title as extra info.
    //No need to delete linked bookmarks. Foreign keys cascades will later delete the links.
    if (!bdata.Ex_LinkedBookmarksList.empty())
    {
        QStringList linkedBookmarkNames;
        success = dbm->bms.RetrieveBookmarkNames(bdata.Ex_LinkedBookmarksList, linkedBookmarkNames);
        if (!success)
            return false;

        //Don't prepend their count to them. Why make it complicated when we already don't want to
        //  link them again and we can't manage '||' in bookmark names in this case? This will
        //  complicate the following 'merge' too.
        //  linkedBookmarkNames.insert(0, QString::number(linkedBookmarkNames.count()));
        //Join them using '||'.
        QString linkedBookmarkNamesStr = linkedBookmarkNames.join("||");
        //Add or merge them into extra info.
        bool hasLinkedBookmarkExInfo = false;
        for (int i = 0; i < bdata.Ex_ExtraInfosList.size(); i++)
        {
            if (bdata.Ex_ExtraInfosList[i].Name == "BM-LinkedBookmarks")
            {
                //The `empty()` check of the parent `if` prevents appending useless '||'s.
                bdata.Ex_ExtraInfosList[i].Type = BookmarkManager::BookmarkExtraInfoData::Type_Text;
                bdata.Ex_ExtraInfosList[i].Value += "||" + linkedBookmarkNamesStr;
                hasLinkedBookmarkExInfo = true;
                break;
            }
        }
        if (!hasLinkedBookmarkExInfo)
        {
            BookmarkManager::BookmarkExtraInfoData exInfo;
            exInfo.Name = "BM-LinkedBookmarks";
            exInfo.Type = BookmarkManager::BookmarkExtraInfoData::Type_Text;
            exInfo.Value = linkedBookmarkNamesStr;
            bdata.Ex_ExtraInfosList.append(exInfo);
        }
    }

    //Convert extra info to one big chunk of json text.
    QJsonArray exInfoJsonArray;
    foreach (const BookmarkManager::BookmarkExtraInfoData& exInfo, bdata.Ex_ExtraInfosList)
    {
        QJsonObject exInfoJsonObject;
        exInfoJsonObject.insert("Name", QJsonValue(exInfo.Name));
        exInfoJsonObject.insert("Type", QJsonValue(BookmarkManager::BookmarkExtraInfoData::DataTypeName(exInfo.Type)));
        exInfoJsonObject.insert("Value", QJsonValue(exInfo.Value));
        exInfoJsonArray.append(QJsonValue(exInfoJsonObject));
    }
    QString extraInfoJSonText = QString::fromUtf8(QJsonDocument(exInfoJsonArray).toJson(QJsonDocument::Compact));

    //Convert folder ID to an absolute folder path.
    BookmarkFolderManager::BookmarkFolderData fodata;
    success = dbm->bfs.RetrieveBookmarkFolder(bdata.FOID, fodata);
    if (!success)
        return false;

    //We use `Ex_AbsolutePath` instead of `GetFileArchiveAndFolderHint` as all files/paths go in one
    //  Trash archive and we want to save the full hierarchy path in this case. This is only in DB;
    //  on file system this is NOT used as folderHint. Trashing file is done before this  line and
    //  it doesn't use a folderHint because folderHint will be ignored on FAM's FileLayout 1 which
    //  is used by the Trash file archive.
    QString folderPath = fodata.Ex_AbsolutePath;

    //Move the information to BookmarkTrash table
    success = dbm->bms.InsertBookmarkIntoTrash(
                folderPath, bdata.Name, bdata.URLs, bdata.Desc, tagNames,
                attachedFIDsStr, defaultFID, bdata.Rating, bdata.AddDate, extraInfoJSonText);
    if (!success)
        return false;

    success = dbm->bms.RemoveBookmark(BID);
    if (!success)
        return false;

    return true;
}

bool BookmarksBusinessLogic::MoveBookmarksToFolderTrans(const QList<long long>& BIDs, long long FOID)
{
    //[Similar BookmarksBusinessLogic Implementation]
    bool success;

    BeginActionTransaction();
    {
        foreach (long long BID, BIDs)
        {
            success = MoveBookmarkToFolder(BID, FOID);
            if (!success)
            {
                RollBackActionTransaction();
                return false; //Always return false
            }
        }
    }
    CommitActionTransaction();

    return success; //i.e `true`.
}

bool BookmarksBusinessLogic::MoveBookmarkToFolderTrans(long long BID, long long FOID)
{
    return MoveBookmarksToFolderTrans(QList<long long>() << BID, FOID);
}

bool BookmarksBusinessLogic::MoveBookmarkToFolder(long long BID, long long FOID)
{
    bool success = true;
    BookmarkManager::BookmarkData bdata;

    //Get bookmark and files info
    success = dbm->bms.RetrieveBookmark(BID, bdata);
    if (!success)
        return false;

    success = dbm->files.RetrieveBookmarkFiles(BID, bdata.Ex_FilesList);
    if (!success)
        return false;

    //If already in the same folder, succeed silently.
    if (bdata.FOID == FOID)
        return true;

    //Update FOID
    bdata.FOID = FOID;
    success = dbm->bms.AddOrEditBookmark(BID, bdata);
    if (!success)
        return false;

    //Get target FOID and folderHint
    QString fileArchiveName, folderHint;
    success = dbm->bfs.GetFileArchiveAndFolderHint(FOID, fileArchiveName, folderHint);
    if (!success)
        return false;

    //Move the files
    foreach (const FileManager::BookmarkFile& bf, bdata.Ex_FilesList)
    {
        success = dbm->files.ChangeFileLocation(bf.FID, fileArchiveName, folderHint, bdata.Name,
                                                "moving bookmark to folder");
        if (!success)
            return false;
    }

    return true;
}

bool BookmarksBusinessLogic::MergeBookmarksTrans(const QList<long long>& BIDs, QList<long long>& associatedTIDs)
{
    //[Similar BookmarksBusinessLogic Implementation]
    bool success;
    QList<long long> eachAssociatedTIDs;

    if (BIDs.count() < 2)
    {
        QMessageBox::warning(dialogParent, "Cannot Merge Bookmarks", "Need at least two bookmarks to merge.");
        return false;
    }

    associatedTIDs.clear(); //Do it for user.
    BeginActionTransaction();
    {
        for (int i = 1; i < BIDs.count(); i++)
        {
            //Merge each bookmark onto the first bookmark
            success = MergeBookmarks(BIDs[0], BIDs[i], eachAssociatedTIDs);
            if (!success)
            {
                RollBackActionTransaction();
                return false; //Always return false
            }

            //Accumulate all associatedTIDs
            foreach (long long TID, eachAssociatedTIDs)
                if (!associatedTIDs.contains(TID))
                    associatedTIDs.append(TID);
        }
    }
    CommitActionTransaction();

    return success; //i.e `true`.
}

bool BookmarksBusinessLogic::MergeBookmarksTrans(long long mainBID, long long subBID, QList<long long>& associatedTIDs)
{
    return MergeBookmarksTrans(QList<long long>() << mainBID << subBID, associatedTIDs);
}

bool BookmarksBusinessLogic::MergeBookmarks(long long mainBID, long long subBID, QList<long long>& associatedTIDs)
{
    //When merging two bookmarks, regarding BookmarkManager.BookmarkData data:
    //  - Sub's Name and Desc will be kept in main's Desc.
    //  - Sub's URLs, ExtraInfos and Files will be appended to main's.
    //  - Sub's LinkedBookmarks and Tags will be compared and merged into main's.
    //  - Things such as Sub's Folder, DefBFID, Rating, AddDate, etc will be forgotten.

    //Retrieve both bookmarks
    bool success = true;
    BookmarkManager::BookmarkData mainBdata;
    BookmarkManager::BookmarkData subBdata;

    success = RetrieveBookmarkEx(mainBID, mainBdata, true, false);
    if (!success)
        return false;

    success = RetrieveBookmarkEx(subBID, subBdata, false, false);
    if (!success)
        return false;

    //Start merging
    //Textual info from sub bookmark
    QString subTextualInfo = QString("Merged Bookmark: %1\n%2\n").arg(subBdata.Name, subBdata.Desc);
    QString mergedDesc = mainBdata.Desc;
    if (mergedDesc.length() > 0)
         mergedDesc += QString("\n\n%1\n").arg(QString(50, '-'));
    mergedDesc += subTextualInfo;

    //URLs: we don't eliminate duplicates or anything
    QString mergedURLs = mainBdata.URLs + '\n' + subBdata.URLs;

    //Linked Bookmarks: add Sub's to Main's. If Sub and Main are linked don't link Main to itself!
    QList<long long> mergedLinkedBIDs = mainBdata.Ex_LinkedBookmarksList;
    foreach (long long subLinkedBID, subBdata.Ex_LinkedBookmarksList)
        if (subLinkedBID != mainBID && !mergedLinkedBIDs.contains(subLinkedBID))
            mergedLinkedBIDs.append(subLinkedBID);

    //ExtraInfos: we have Main's model and Sub's list. We merge them and add all [same-name ExtraInfos].
    foreach (const BookmarkManager::BookmarkExtraInfoData& exInfo, subBdata.Ex_ExtraInfosList)
    {
        success = dbm->bms.InsertBookmarkExtraInfoIntoModel(
                    mainBdata.Ex_ExtraInfosModel, mainBID, exInfo.Name, exInfo.Type, exInfo.Value);
        if (!success)
            return false;
    }

    //Tags: We just copy them over, when saving duplicates are eliminated.
    QStringList mergedTagsList = mainBdata.Ex_TagsList;
    mergedTagsList.append(subBdata.Ex_TagsList);

    //BookmarkFiles: Append Sub's files to Main's.
    //Documentation moved to [Merging Bookmark Files]. Code samples were removed, but are available
    //in revision a7cf3e9a at 2017-06-01. The following implements solution (D).
    QList<FileManager::BookmarkFile> mergedFilesList = mainBdata.Ex_FilesList;
    foreach (const FileManager::BookmarkFile& subBf, subBdata.Ex_FilesList)
    {
        FileManager::BookmarkFile sharedBf = subBf;
        sharedBf.BFID = -1; //Still have a valid FID; this will share the file.
        sharedBf.Ex_SharedFileLocationPolicy = FileManager::BookmarkFile::SFLP_MoveToNewLocation;
        mergedFilesList.append(sharedBf);
    }

    //Set default file index. We assume if Main or Sub have files their DefBFID will not be -1.
    //Related to [KeepDefaultFile-1].Generalization: if no files, `defaultFileIndex` will be -1.
    int defaultFileIndex = -1;
    int defaultFileBFID = (mainBdata.DefBFID != -1 ? mainBdata.DefBFID : subBdata.DefBFID);
    //We can't operate on mergedFilesList as its BFIDs are set to -1 above. So we first check
    //Main's, and if still not found, Sub's files list (note the second for's condition:
    //`defaultFileIndex == -1` causes to search Sub's files if file has been found in Main.)
    for (int i = 0; defaultFileIndex == -1 && i < mainBdata.Ex_FilesList.count(); i++)
        if (mainBdata.Ex_FilesList[i].BFID == defaultFileBFID)
            defaultFileIndex = i;
    for (int i = 0; defaultFileIndex == -1 && i < subBdata.Ex_FilesList.count(); i++)
        if (subBdata.Ex_FilesList[i].BFID == defaultFileBFID)
            defaultFileIndex = i;

    //Edit Main to contain the merged values.
    BookmarkManager::BookmarkData bdata;
    bdata.BID = mainBdata.BID;
    bdata.FOID = mainBdata.FOID;
    bdata.Name = mainBdata.Name;
    bdata.URLs = mergedURLs;
    bdata.Desc = mergedDesc;
    bdata.DefBFID = -1; //[KeepDefaultFile-1] We always set this to -1.
                        //bbLogic will set the correct defbfid later.
    bdata.Rating = mainBdata.Rating;

    //Apply the edit
    success = AddOrEditBookmark(
                mainBID, bdata, mainBID, mainBdata, mergedLinkedBIDs,
                QList<BookmarkManager::BookmarkExtraInfoData>() /* mainBdata.Ex_ExtraInfosModel will be used */,
                mergedTagsList, associatedTIDs, mergedFilesList, defaultFileIndex);
    if (!success)
        return false;

    //Remove Sub and its associated data. Will not delete its files as they are shared with Main.
    success = DeleteBookmark(subBID); //NOT: dbm->bms.RemoveBookmark(subBID);
    if (!success)
        return false;

    return true;
}
