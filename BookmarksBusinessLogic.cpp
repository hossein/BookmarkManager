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

bool BookmarksBusinessLogic::AddOrEditBookmark(long long& editBId, BookmarkManager::BookmarkData& bdata,
                                               long long originalEditBId, BookmarkManager::BookmarkData& editOriginalBData,
                                               const QList<long long>& editedLinkedBookmarks,
                                               const QStringList& tagsList, QList<long long>& associatedTIDs,
                                               const QList<FileManager::BookmarkFile>& editedFilesList, int defaultFileIndex)
{
    bool success;

    //We do NOT use the transactions in a nested manner. This 'linear' ('together') mode is was
    //  not only and easier to understand, it's better for error management, too.
    //IMPORTANT: In case of RollBack, do NOT return!
    //           [why-two-editbids]: In case of ADDing, MUST set editBId to its original `-1` value!
    //           Thanksfully FilesList don't need changing.
    dbm->db.transaction();
    dbm->files.BeginFilesTransaction();
    {
        success = dbm->bms.AddOrEditBookmark(editBId, bdata); //For Add, the editBID will be modified!
        if (!success)
            return DoRollBackAction(editBId, originalEditBId);

        success = dbm->bms.UpdateLinkedBookmarks(editBId, editOriginalBData.Ex_LinkedBookmarksList,
                                                 editedLinkedBookmarks);
        if (!success)
            return DoRollBackAction(editBId, originalEditBId);

        //We use models, this function is no longer necessary.
        //success = dbm->bms.UpdateBookmarkExtraInfos(editBId, editOriginalBData.Ex_ExtraInfosList,
        //                                            editedExtraInfos);
        //DO NOT: success = editOriginalBData.Ex_ExtraInfosModel.submitAll(); Read docs
        success = dbm->bms.UpdateBookmarkExtraInfos(editBId, editOriginalBData.Ex_ExtraInfosModel);
        if (!success)
            return DoRollBackAction(editBId, originalEditBId);

        success = dbm->tags.SetBookmarkTags(editBId, tagsList, associatedTIDs);
        if (!success)
            return DoRollBackAction(editBId, originalEditBId);

        QList<long long> updatedBFIDs;
        success = dbm->files.UpdateBookmarkFiles(editBId, bdata.Name,
                                                 editOriginalBData.Ex_FilesList, editedFilesList,
                                                 updatedBFIDs, dbm->conf->currentFileArchiveForAddingFiles);
        if (!success)
            return DoRollBackAction(editBId, originalEditBId);

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
            return DoRollBackAction(editBId, originalEditBId);
    }
    dbm->files.CommitFilesTransaction(); //Committing files transaction doesn't fail!
    dbm->db.commit(); //Assume doesn't fail

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

    //IMPORTANT: Begin transaction.
    dbm->db.transaction();
    dbm->files.BeginFilesTransaction();
    {
        //Convert attached file FIDs to CSV string.
        //Get the FID of the default file
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

        //long long defaultFID = -1;
        //if (bdata.DefBFID == -1) We can NOT guarantee this if user has removed files of a bookmark. TODO: When user removes files, does default change? when user removes the last file, does it become -1? If yes we can add the following lines:
        ////Generalization: If a bookmark doesn't have any files, set its DefBFID to -1.
        ////      This can be used when trashing to detect whether any file exists or not, and whether we can find the FID of it.
        //to [KeepDefaultFile-1] explanation. and can use that condition then.
        //// BUT UPDATE AGAIN: Now we use the previous loop to find the def FID anyway and DO NOT NEED SUCH GUARANTEES.

        //Remove BookmarkFile attachment information. This is necessary as foreign keys restrict
        //  deleting a bookmark having associated files with it.
        //Send files to trash (if they're not shared).
        success = dbm->files.TrashAllBookmarkFiles(BID);
        if (!success)
            return DoRollBackAction();

        //Convert tag names to csv.
        //Foreign keys cascades will later delete the tags.
        QString tagNames = bdata.Ex_TagsList.join(",");

        //Convert extra info to one big chunk of json text.
        //TODO: Make linked bookmarks titles extra info.
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

        //Do nothing about linked bookmarks. Foreign keys cascades will later delete the links.
        //Okay!

        //Move the information to BookmarkTrash table
        success = dbm->bms.InsertBookmarkIntoTrash(bdata.Name, bdata.URL, bdata.Desc, tagNames,
                                                   attachedFIDsStr, defaultFID, bdata.Rating,
                                                   bdata.AddDate, extraInfoJSonText);
        if (!success)
            return DoRollBackAction();

        success = dbm->bms.RemoveBookmark(BID);
        if (!success)
            return DoRollBackAction();
    }
    dbm->files.CommitFilesTransaction(); //Committing files transaction doesn't fail!
    dbm->db.commit(); //Assume doesn't fail

    return success; //i.e `true`.
}

bool BookmarksBusinessLogic::DoRollBackAction()
{
    dbm->db.rollback();

    bool rollbackResult = dbm->files.RollBackFilesTransaction();
    if (!rollbackResult)
    {
        const QString errorText = "Not all changes made to your filesystem in the intermediary "
                                  "process of adding the bookmark could not be reverted.<br/><br/>"
                                  "<b>Your filesystem may be in inconsistent state!</b>";
        qDebug() << "File Transaction Rollback Error\n" << errorText;
        QMessageBox::critical(dialogParent, "File Transaction Rollback Error", errorText);
    }

    return false;
}

bool BookmarksBusinessLogic::DoRollBackAction(long long& editBId, const long long originalEditBId)
{
    editBId = originalEditBId; //Affects only the Adding mode.
    return DoRollBackAction();
}
