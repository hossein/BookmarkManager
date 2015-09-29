#include "BookmarksBusinessLogic.h"

#include "Config.h"
#include "DatabaseManager.h"

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
        const QStringList& tagsList, QList<long long>& associatedTIDs,
        const QList<FileManager::BookmarkFile>& editedFilesList, int defaultFileIndex)
{
    bool success;

    //IMPORTANT: In case of RollBack, do NOT return!
    //           [why-two-editbids]: In case of ADDing, MUST set editBId to its original `-1` value!
    //           Thanksfully FilesList don't need changing.
    BeginActionTransaction();
    {
        success = AddOrEditBookmark(editBId, bdata, originalEditBId, editOriginalBData,
                                     editedLinkedBookmarks, tagsList, associatedTIDs,
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

    //We use models, this function is no longer necessary.
    //success = dbm->bms.UpdateBookmarkExtraInfos(editBId, editOriginalBData.Ex_ExtraInfosList,
    //                                            editedExtraInfos);
    //DO NOT: success = editOriginalBData.Ex_ExtraInfosModel.submitAll(); Read docs
    success = dbm->bms.UpdateBookmarkExtraInfos(editBId, editOriginalBData.Ex_ExtraInfosModel);
    if (!success)
        return false;

    success = dbm->tags.SetBookmarkTags(editBId, tagsList, associatedTIDs);
    if (!success)
        return false;

    QList<long long> updatedBFIDs;
    success = dbm->files.UpdateBookmarkFiles(editBId, bdata.Name,
                                             editOriginalBData.Ex_FilesList, editedFilesList,
                                             updatedBFIDs, dbm->conf->currentFileArchiveForAddingFiles,
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

bool BookmarksBusinessLogic::DeleteBookmarkTrans(long long BID)
{
    bool success;

    //IMPORTANT: Begin transaction.
    BeginActionTransaction();
    {
        success = DeleteBookmark(BID);
        if (!success)
        {
            RollBackActionTransaction();
            return false; //Always return false
        }
    }
    CommitActionTransaction();

    return success; //i.e `true`.

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
    //  8. Move the information to BookmarkTrash table.

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

    //Stor linked bookmarks' title as extra info.
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

    //Move the information to BookmarkTrash table
    success = dbm->bms.InsertBookmarkIntoTrash(bdata.Name, bdata.URL, bdata.Desc, tagNames,
                                               attachedFIDsStr, defaultFID, bdata.Rating,
                                               bdata.AddDate, extraInfoJSonText);
    if (!success)
        return false;

    success = dbm->bms.RemoveBookmark(BID);
    if (!success)
        return false;
}
