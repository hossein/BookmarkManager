#include "BookmarkImporter.h"

#include "BookmarksBusinessLogic.h"
#include "FileArchiveManager.h"
#include "Util.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

BookmarkImporter::BookmarkImporter(DatabaseManager* dbm, QWidget* dialogParent)
    : dbm(dbm), m_dialogParent(dialogParent)
{

}

bool BookmarkImporter::Initialize()
{
    //Query bookmark urls
    QHash<long long, QString> bookmarkURLs;
    bool success = dbm->bms.RetrieveAllFullURLs(bookmarkURLs);
    if (!success)
        return false;

    existentBookmarksForUrl.clear();
    for (auto it = bookmarkURLs.cbegin(); it != bookmarkURLs.cend(); ++it)
        existentBookmarksForUrl.insertMulti(GetURLForFastComparison(it.value()), it.key());

    //Query bookmark unique ids.
    //[No-Firefox-Uniqure-Ids]
    //Note: Until e7d886fd2227165c67cd61e75137622ada874e4c @ 20141116 we queried `firefox-guid` and
    //      wanted to query other browsers' unique ids too; but it appears that guid's of firefox
    //      are not what their name suggests. There may be multiple bookmarks with the same url but
    //      different guids that are in different folders representing firefox bookmarks' tags, and
    //      not only guid but every property, including the adding and modify dates differs on them.
    //Also if unique ids match but urls dont then don't assume bookmarks are equal. So what was its
    //      usage after all? So it was removed. (btw it might not be called 'firefox-guid' in newer
    //      revisions.)
    //Also, update: guid seems to be an attribute with firefox's sync or sth. It doesn't exist on
    //      older firefox's bookmarks backup files.

    return true;
}

bool BookmarkImporter::Analyze(ImportedEntityList& elist)
{
    //First initialize the data we will need during the conversion.
    for (int i = 0; i < elist.ibflist.size(); i++)
        folderItemsIndexInArray[elist.ibflist[i].intId] = i;

    //Tag all the bookmarks
    QString tagByParentFolder;
    for (int index = 0; index < elist.iblist.size(); index++)
    {
        elist.iblist[index].Ex_additionalTags = QStringList();
        tagByParentFolder = bookmarkTagAccordingToParentFolders(elist, index);
        if (!tagByParentFolder.isEmpty())
            elist.iblist[index].Ex_additionalTags.append(tagByParentFolder);
    }

    //Find URLs of the TO-BE-IMPORTED bookmarks that EXACTLY match.
    //  This is done to merge firefox bookmarks with different tags into one bookmark.
    //Insert everything into a multihash
    QMultiHash<QString, int> exactURLIndices;
    for (int i = 0; i < elist.iblist.size(); i++)
        exactURLIndices.insertMulti(elist.iblist[i].uri, i);

    //Find those keys who have more than one bookmark.
    QStringList duplicateExactURLs;
    foreach (const QString& exactURL, exactURLIndices.uniqueKeys())
        if (exactURLIndices.count(exactURL) > 1)
            duplicateExactURLs.append(exactURL);

    //Collect all descriptions and titles for each duplicate url; mark one of them as 'original' and remove the rest.
    //I saw that usually among the tagged duplicate bookmarks, just one of them contains title and description
    //  and the rest are without title and description. But anyway we use a join operation to be safe.
    QList<int> indicesToDelete;
    foreach (const QString& duplicateURL, duplicateExactURLs)
    {
        const QList<int>& indicesForURL = exactURLIndices.values(duplicateURL);

        int originalIndex = indicesForURL[0]; //The one with title and/or description.
        QStringList titles;
        QStringList descriptions;
        QStringList tags;
        foreach (int index, indicesForURL)
        {
            const ImportedBookmark& ib = elist.iblist[index];
            if (!ib.title.isEmpty() || !ib.description.isEmpty())
                originalIndex = index;
            if (!ib.title.isEmpty())
                titles.append(ib.title);
            if (!ib.description.isEmpty())
                descriptions.append(ib.description);
            foreach (const QString& tag, ib.Ex_additionalTags)
                if (!tags.contains(tag, Qt::CaseSensitive))
                    tags.append(tag);
        }

        //Apply the new title, descriptions, tags to the only bookmark that is going to remain.
        elist.iblist[originalIndex].title = titles.join(" -- ");
        elist.iblist[originalIndex].description = descriptions.join("\n\n");
        elist.iblist[originalIndex].Ex_additionalTags = tags;

        //Add those we eant to remove all except the one with the most information (at originalIndex).
        //We can't just remove them here, it will change the indexes of bookmarks in the next iterations.
        indicesToDelete.append(indicesForURL);
        indicesToDelete.removeOne(originalIndex);
    }

    qSort(indicesToDelete);
    for (int i = indicesToDelete.size() - 1; i >= 0; i--)
        elist.iblist.removeAt(indicesToDelete[i]);

    //Convert text in brackets in titles to descriptions, like I write them.
    //  Do this before doing the similarity check below.
    QString brTitle, brDesc;
    for (int i = 0; i < elist.iblist.size(); i++)
    {
        //By reference.
        ImportedBookmark& ib = elist.iblist[i];
        BreakTitleBracketedDescription(ib.title, brTitle, brDesc);
        if (!brTitle.isEmpty() || !brDesc.isEmpty())
        {
            ib.title = brTitle;
            if (ib.description.isEmpty())
                ib.description = brDesc;
            else
                ib.description += "\n" + brDesc;
        }
    }

    //Now check the urls for duplicates among EXISTING bookmarks.
    for (int i = 0; i < elist.iblist.size(); i++)
    //foreach (ImportedBookmark& ib, elist.iblist)
    {
        //By reference.
        ImportedBookmark& ib = elist.iblist[i];

        QString fastDuplCheckURL = GetURLForFastComparison(ib.uri);
        if (existentBookmarksForUrl.contains(fastDuplCheckURL))
        {
            //Whoops, we found a similar match until now. Check if it's fully similar or just partially.
            QList<long long> existentAlmostDuplicateBIDs = existentBookmarksForUrl.values(fastDuplCheckURL);

            bool foundSimilar;
            bool foundExact;
            long long duplicateBID;
            bool duplicateFindSuccess = FindDuplicate(ib, existentAlmostDuplicateBIDs,
                                                      foundSimilar, foundExact, duplicateBID);
            if (!duplicateFindSuccess)
                return false;

            if (foundSimilar || foundExact)
            {
                if (foundExact)
                    ib.Ex_status = ImportedBookmark::S_AnalyzedExactExistent;
                else //if (foundSimilar
                    ib.Ex_status = ImportedBookmark::S_AnalyzedSimilarExistent;

                ib.Ex_AlmostDuplicateExistentBIDs = existentBookmarksForUrl.values(fastDuplCheckURL);
                ib.Ex_DuplicateExistentComparedBID = duplicateBID;

                continue; //These `if`s don't have `else`s, don't reach the parts after them that set them to ImportOK.
            }
        }

        //If urls are not similar
        ib.Ex_status = ImportedBookmark::S_AnalyzedImportOK;
    }

    //Bookmarks imported; delete folders without bookmarks or folders in them until nothing remains to be deleted.
    //Note: There is a 'Tags' folder which should automatically be deleted because it just contains bookmarks
    //  without titles or anything but just tags. However in case of corrupt files or user manipulation, its
    //  bookmarks will end up as a bunch of [title-less bookmarks] which we will handle without a problem.
    QSet<int> usedFolderIds;
    while (true)
    {
        usedFolderIds.clear();
        foreach (const ImportedBookmark& ib, elist.iblist)
            usedFolderIds.insert(ib.parentId);
        foreach (const ImportedBookmarkFolder& ibf, elist.ibflist)
            usedFolderIds.insert(ibf.parentId);

        indicesToDelete.clear();
        for (int i = 0; i < elist.ibflist.size(); i++)
            if (!usedFolderIds.contains(elist.ibflist[i].intId))
                indicesToDelete.append(i);

        if (indicesToDelete.isEmpty())
            break;

        qSort(indicesToDelete);
        for (int i = indicesToDelete.size() - 1; i >= 0; i--)
            elist.ibflist.removeAt(indicesToDelete[i]);
    }

    return true;
}

bool BookmarkImporter::Import(ImportedEntityList& elist, QList<long long>& addedBIDs,
                              QSet<long long>& allAssociatedTIDs,
                              QList<ImportedBookmark*>& failedProcessOrImports)
{
    //Do it for caller
    addedBIDs.clear();
    allAssociatedTIDs.clear();
    failedProcessOrImports.clear();

    if (!InitializeImport())
        return false;

    foreach (const ImportedBookmark& ib, elist.iblist)
    {
        if (!ImportOne(ib))
            continue; //Actually never return. Import all the bookmarks.
    }

    FinalizeImport(addedBIDs, allAssociatedTIDs, failedProcessOrImports);

    return true;
}

bool BookmarkImporter::InitializeImport()
{
    m_addedBIDs.clear();
    m_allAssociatedTIDs.clear();
    m_failedProcessOrImports.clear();

    m_tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/BMTemp";
    if (!QDir().mkpath(m_tempPath))
    {
        QMessageBox::critical(
                    m_dialogParent, "Error",
                    QString("Could not create the temp directory \"%1\". No bookmark was imported.")
                    .arg(m_tempPath));
        return false;
    }
    if (!Util::RemoveDirectoryRecursively(m_tempPath, false /* don't remove the directory itself */))
    {
        QMessageBox::critical(
                    m_dialogParent, "Error",
                    QString("Could not empty the temp directory \"%1\". No bookmark was imported.")
                    .arg(m_tempPath));
        return false;
    }

    return true;
}

bool BookmarkImporter::ImportOne(const ImportedBookmark& ib)
{
    if (!ib.Ex_finalImport)
        //Marked by user to not import.
        return true;

    //Important: We don't want to put the attached file in recycle bin. So we will manually
    //  delete it instead of using BookmarkFile::Ex_RemoveAfterAttach.
    QString mhtFilePathName;
    QList<FileManager::BookmarkFile> bookmarkFiles;
    if (ib.ExPr_attachedFileError.isEmpty())
    {
        QString safeFileName = FileArchiveManager::SafeAndShortFSName(ib.ExPr_attachedFileName, true);
        mhtFilePathName = m_tempPath + "/" + safeFileName;
        QFile mhtfile(mhtFilePathName);
        if (!mhtfile.open(QIODevice::WriteOnly))
        {
            QMessageBox::warning(
                        m_dialogParent, "Error",
                        QString("Could not create the temp file \"%1\" for bookmark \"%2\" (\"%3\").\nError: %4 %5")
                        .arg(mhtFilePathName, ib.title, ib.uri, QString::number(mhtfile.error()), mhtfile.errorString()));
            return false;
        }
        if (-1 == mhtfile.write(ib.ExPr_attachedFileData))
        {
            QMessageBox::warning(
                        m_dialogParent, "Error",
                        QString("Could not write data to the temp file \"%1\" for bookmark \"%2\" (\"%3\").\nError: %4 %5")
                        .arg(mhtFilePathName, ib.title, ib.uri, QString::number(mhtfile.error()), mhtfile.errorString()));
            mhtfile.close();

            RemoveTempFileIfExists(mhtFilePathName);
            return false;
        }
        if (!mhtfile.flush())
        {
            QMessageBox::warning(
                        m_dialogParent, "Error",
                        QString("Could not flush data to the temp file \"%1\" for bookmark \"%2\" (\"%3\").\nError: %4 %5")
                        .arg(mhtFilePathName, ib.title, ib.uri, QString::number(mhtfile.error()), mhtfile.errorString()));
            mhtfile.close();

            RemoveTempFileIfExists(mhtFilePathName);
            return false;
        }
        mhtfile.close();

        //Same logic exists in BookmarkEditDialog::accept and its file attaching functions.
        QFileInfo fileInfo(mhtFilePathName);
        FileManager::BookmarkFile bf;
        bf.BFID         = -1; //Leave to FileManager.
        bf.BID          = -1; //Not added yet
        bf.FID          = -1; //Leave to FileManager.
        bf.OriginalName = mhtFilePathName;
        bf.ArchiveURL   = ""; //Leave to FileManager.
        bf.ModifyDate   = fileInfo.lastModified();
        bf.Size         = fileInfo.size();
        bf.MD5          = Util::GetMD5HashForFile(mhtFilePathName);
        bf.Ex_IsDefaultFileForEditedBookmark = true; //[KeepDefaultFile-1].Generalization: Must be set
        bf.Ex_RemoveAfterAttach = false; //We don't want to put it in recycle bin. Will manually delete.

        bookmarkFiles.append(bf);
    }

    if (ib.Ex_status == ImportedBookmark::S_AnalyzedImportOK)
    {
        //[KeepDefaultFile-1].Generalization: Any bookmark MUST have a default file if it has files.
        //  (If it doesn't have files just pass -1, otherwise you may crash AddOrEditBookmark.)
        int defaultFileIndex = (bookmarkFiles.empty() ? -1 : 0);

        BookmarkManager::BookmarkData bdata;
        bdata.BID = -1; //Not important.
        bdata.FOID = 0; //We always import into the '0, Unsorted bookmarks' folder.
        bdata.Name = ib.title.trimmed(); //[title-less bookmarks] are not possible after processing.
        bdata.URL = ib.uri;
        bdata.Desc = ib.description;
        bdata.DefBFID = -1; //[KeepDefaultFile-1] We always set this to -1.
                            //bbLogic will set the correct defbfid later.
        bdata.Rating = 50; //For all.

        long long addedBID = -1; //Must set to -1 to show adding.
        QList<long long> associatedTIDs;
        //We just need an empty thing for 'updating' functions. No need to initialize.
        BookmarkManager::BookmarkData editOriginalBData;

        BookmarksBusinessLogic bbLogic(dbm, m_dialogParent);
        bbLogic.BeginActionTransaction();

        bool success = bbLogic.AddOrEditBookmark(
                    addedBID, bdata, -1, editOriginalBData, QList<long long>(),
                    ib.Ex_finalTags, associatedTIDs, bookmarkFiles, defaultFileIndex);
        if (!success)
        {
            //A messagebox must have already been displayed.
            bbLogic.RollBackActionTransaction();

            RemoveTempFileIfExists(mhtFilePathName);
            return false;
        }

        success = dbm->bms.UpdateBookmarkExtraInfos(
                    addedBID, QList<BookmarkManager::BookmarkExtraInfoData>(), ib.ExPr_ExtraInfosList);
        if (!success)
        {
            //A messagebox must have already been displayed.
            bbLogic.RollBackActionTransaction();

            RemoveTempFileIfExists(mhtFilePathName);
            return false;
        }

        bbLogic.CommitActionTransaction();

        //Success!
        m_addedBIDs.append(addedBID);
        m_allAssociatedTIDs.unite(QSet<long long>::fromList(associatedTIDs));
    }
    else if (ib.Ex_status == ImportedBookmark::S_AnalyzedExactExistent)
    {
        //WE SHOULD NEVER REACH HERE. ib.Ex_finalImport must have been set to false by PreviewDialog
        //  for these bookmarks.
        //We don't need to do anything! Bookmark already in DB.
        //We don't even update its mht file.
    }
    else if (ib.Ex_status == ImportedBookmark::S_ReplaceExisting)
    {
        //TODO: Implement
        QMessageBox::warning(
                    m_dialogParent, "Error",
                    QString("ReplaceExisting NOT implemented! for imported bookmark \"%1\" (\"%2\").")
                    .arg(ib.title, ib.uri));
    }
    else if (ib.Ex_status == ImportedBookmark::S_AppendToExisting)
    {
        //TODO: Implement
        QMessageBox::warning(
                    m_dialogParent, "Error",
                    QString("AppendToExisting NOT implemented! for imported bookmark \"%1\" (\"%2\").")
                    .arg(ib.title, ib.uri));
    }
    else
    {
        QMessageBox::warning(
                    m_dialogParent, "Error",
                    QString("Unknown bookmark import status %1 for imported bookmark \"%2\" (\"%3\").")
                    .arg(QString::number((int)ib.Ex_status), ib.title, ib.uri));
    }

    //There are `return false`s in the code. Everything here must be repeated before them.
    //We always need to remove this file, because we create it regardless of import type.
    //We don't check its return value.
    RemoveTempFileIfExists(mhtFilePathName);

    return true;
}

void BookmarkImporter::MarkAsFailed(ImportedBookmark* ib)
{
    m_failedProcessOrImports.insert(ib);
}

void BookmarkImporter::FinalizeImport(QList<long long>& addedBIDs, QSet<long long>& allAssociatedTIDs,
                                      QList<ImportedBookmark*>& failedProcessOrImports)
{
    addedBIDs = m_addedBIDs;
    allAssociatedTIDs = m_allAssociatedTIDs;
    failedProcessOrImports = m_failedProcessOrImports.toList();
}

QString BookmarkImporter::bookmarkTagAccordingToParentFolders(ImportedEntityList& elist, int bookmarkIndex)
{
    const ImportedBookmark& ib = elist.iblist[bookmarkIndex];
    int parentId = ib.parentId;

    QString tag = QString();
    while (elist.ibflist[folderItemsIndexInArray[parentId]].root.isEmpty())
    {
        tag = elist.ibflist[folderItemsIndexInArray[parentId]].title + (tag.isEmpty() ? "" : "/") + tag;
        parentId = elist.ibflist[folderItemsIndexInArray[parentId]].parentId;
    }
    tag = tag.replace(' ', '-');

    return tag;
}

void BookmarkImporter::BreakTitleBracketedDescription(const QString& titleDesc, QString& title, QString& desc)
{
    //Do it for caller
    title = QString();
    desc = QString();

    QString td = titleDesc.trimmed();
    if (td.isEmpty())
        return;

    if (td[td.length() - 1] != ']')
        return;

    //Try to find a starting '[' at the same level.
    int i;
    bool found = false;
    int level = 0; //Don't make this -1 and start at `td.length() - 2`; we haven't checked length!
    for (i = td.length() - 1; i >= 0; i--)
    {
        if (td[i] == '[')
        {
            level += 1;
            if (level == 0)
            {
                //No need to make sure this isn't the last char; we are sure last char is ']'.
                found = true;
                break;
            }
        }
        else if (td[i] == ']')
        {
            level -= 1;
        }
    }

    if (found)
    {
        title = td.left(i).trimmed();

        desc = td.mid(i + 1); //Skip '['
        desc.chop(1); //Chop ']'
        desc = desc.trimmed();
    }
}

bool BookmarkImporter::FindDuplicate(const ImportedBookmark& ib, const QList<long long>& almostDuplicateBIDs,
                                     bool& foundSimilar, bool& foundExact, long long& duplicateBID)
{
    foundSimilar = false;
    foundExact = false;
    duplicateBID = -1;

    BookmarkManager::BookmarkData bdata;

    foreach (long long existentBID, almostDuplicateBIDs)
    {
        bool retrieveSuccess = dbm->bms.RetrieveBookmark(existentBID, bdata);
        if (!retrieveSuccess)
            return false;

        QString newAlmostExactDuplCheckURL = GetURLForAlmostExactComparison(ib.uri);
        QString existentAlmostExactDuplCheckURL = GetURLForAlmostExactComparison(bdata.URL);

        if (newAlmostExactDuplCheckURL == existentAlmostExactDuplCheckURL)
        {
            //Til now we have found a similar duplicate. Set the BID to the FIRST FOUND similar BID.
            foundSimilar = true;
            if (duplicateBID == -1)
                duplicateBID = existentBID;

            //Exact URL match. Check all the properties for exact match also.
            //Checking to make sure this is not one of [title-less bookmarks] is necessary to avoid
            //  clashing with the existent bookmark where an automatic (or user-selected) title was
            //  assigned.
            //`.trimmed()` is necesessary, at least for `ib`.
            bool detailsMatch = true;
            if (!ib.title.trimmed().isEmpty())
                detailsMatch = detailsMatch && (ib.title.trimmed() == bdata.Name.trimmed());

            //Note: To generalize the code below, at least one of the guids of e.g Tagged bookmarks
            //  must match. Probably can't check with single-line if's.
            //So this was removed due to [No-Firefox-Uniqure-Ids], and also because it's unneeded.
            //  QList<BookmarkManager::BookmarkExtraInfoData> extraInfos;
            //  retrieveSuccess = dbm->bms.RetrieveBookmarkExtraInfos(existentBID, extraInfos);
            //  if (!retrieveSuccess)
            //      return false;
            //  QString ffGuidField = extraInfoField("firefox-guid", extraInfos);
            //  detailsMatch = detailsMatch && (!ffGuidField.isNull() && ib.guid == ffGuidField);

            //The isEmpty check makes sure we don't unnecessarily nag on a bookmark where user has
            //  added desctiptions themselves after previous import.
            if (!ib.description.trimmed().isEmpty())
                detailsMatch = detailsMatch && (ib.description.trimmed() == bdata.Desc.trimmed());

            detailsMatch = detailsMatch && (ib.uri == bdata.URL);

            if (detailsMatch)
            {
                //Found an exact match. Don't check the rest of the list.
                foundSimilar = false; //If any
                foundExact = true;
                duplicateBID = existentBID;
                break;
            }
        }
    }

    return true;
}

QString BookmarkImporter::GetURLForFastComparison(const QString& originalUrl)
{
    //These work okay with "file:" and "mailto:" urls.
    QUrl url(originalUrl);
    return QString(url.host() + '/' + url.path()).toLower();
}

QString BookmarkImporter::GetURLForAlmostExactComparison(const QString& originalUrl)
{
    //These work okay with "file:" and "mailto:" urls.
    QUrl url(originalUrl);
    return url.host() + '/' + url.path() + '?' + url.query() + '#' + url.fragment();
}

QString BookmarkImporter::extraInfoField(const QString& fieldName, const QList<BookmarkManager::BookmarkExtraInfoData>& extraInfos)
{
    foreach (const BookmarkManager::BookmarkExtraInfoData& exInfo, extraInfos)
        if (exInfo.Name == fieldName)
            return exInfo.Value;
    return QString();
}

void BookmarkImporter::RemoveTempFileIfExists(const QString& filePathName)
{
    if (filePathName.isEmpty())
        return;

    QFile f(filePathName);
    //`exists` return false if file is an existing symlink to a nonexisting file
    if (!f.exists() && f.symLinkTarget().isEmpty())
        return;

    if (!f.remove())
    {
        QMessageBox::warning(
                    m_dialogParent, "Error",
                    QString("Could not remove the temporary saved page file \"%1\". "
                            "Future errors might happen because of this.\nError: %2 %3")
                    .arg(filePathName, QString::number(f.error()), f.errorString()));
    }
}
