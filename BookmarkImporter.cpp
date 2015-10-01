#include "BookmarkImporter.h"

#include "BookmarksBusinessLogic.h"
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

    return true;
}

bool BookmarkImporter::Analyze(ImportedEntityList& elist)
{
    //First initialize the data we will need during the conversion.
    for (int i = 0; i < elist.ibflist.size(); i++)
        folderItemsIndexInArray[elist.ibflist[i].intId] = i;

    //Tag all the bookmarks
    for (int index = 0; index < elist.iblist.size(); index++)
        elist.iblist[index].Ex_additionalTags = QStringList(bookmarkTagAccordingToParentFolders(elist, index));

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
            long long existentBID = existentBookmarksForUrl.value(fastDuplCheckURL);

            BookmarkManager::BookmarkData bdata;
            bool retrieveSuccess = dbm->bms.RetrieveBookmark(existentBID, bdata);
            if (!retrieveSuccess)
                return false;

            QString newAlmostExactDuplCheckURL = GetURLForAlmostExactComparison(ib.uri);
            QString existentAlmostExactDuplCheckURL = GetURLForAlmostExactComparison(bdata.URL);

            if (newAlmostExactDuplCheckURL == existentAlmostExactDuplCheckURL)
            {
                //Exact URL match. Check all the properties for exact match also.
                QList<BookmarkManager::BookmarkExtraInfoData> extraInfos;
                retrieveSuccess = dbm->bms.RetrieveBookmarkExtraInfos(existentBID, extraInfos);
                if (!retrieveSuccess)
                    return false;

                bool detailsMatch = true;
                detailsMatch = detailsMatch && (ib.title == bdata.Name);

                //Note: To generalize the code below, at least one of the guids must match.
                //  Probably can't check with single-line if's.
                //This was removed due to [No-Firefox-Uniqure-Ids], and also because this is unneeded.
                //QString ffGuidField = extraInfoField("firefox-guid", extraInfos);
                //detailsMatch = detailsMatch && (!ffGuidField.isNull() && ib.guid == bdata.Name);

                detailsMatch = detailsMatch && (ib.description == bdata.Desc);
                detailsMatch = detailsMatch && (ib.uri == bdata.URL);

                if (detailsMatch)
                    ib.Ex_status = ImportedBookmark::S_AnalyzedExactExistent;
                else
                    ib.Ex_status = ImportedBookmark::S_AnalyzedSimilarExistent;

                ib.Ex_DuplicateExistentBIDs = existentBookmarksForUrl.values(fastDuplCheckURL);
                ib.Ex_DuplicateExistentComparedBID = existentBID;

                continue; //These `if`s don't have `else`s, don't reach the parts after them and set them to ImportOK.
            }
        }

        //If urls are not similar
        ib.Ex_status = ImportedBookmark::S_AnalyzedImportOK;
    }

    //Bookmarks imported; delete folders without bookmarks or folders in them until nothing remains to be deleted.
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

bool BookmarkImporter::Import(ImportedEntityList& elist)
{
    QList<long long> addedBIDs;
    QSet<long long> allAssociatedTIDs;

    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/BMTemp";
    if (!QDir().mkpath(tempPath))
    {
        QMessageBox::critical(
                    m_dialogParent, "Error",
                    QString("Could not create the temp directory \"%1\". No bookmark was imported.")
                    .arg(tempPath));
        return false;
    }

    foreach (const ImportedBookmark& ib, elist.iblist)
    {
        if (!ib.Ex_finalImport)
            //Marked by user to not import.
            continue;

        QList<FileManager::BookmarkFile> bookmarkFiles;
        if (ib.ExPr_attachedFileError.isEmpty())
        {
            //Same logic is used in FAM.
            QString safeFileName = Util::PercentEncodeUnicodeAndFSChars(ib.ExPr_attachedFileName);
            if (safeFileName.length() > 64) //For WINDOWS!
            {
                const QFileInfo safeFileNameInfo(safeFileName);
                const QString cbaseName = safeFileNameInfo.completeBaseName();
                safeFileName = cbaseName.left(qMin(64, cbaseName.length()));
                if (!safeFileNameInfo.suffix().isEmpty())
                    safeFileName += "." + safeFileNameInfo.suffix();
            }

            QString mhtFilePathName = tempPath + "/" + safeFileName;
            QFile mhtfile(mhtFilePathName);
            if (!mhtfile.open(QIODevice::WriteOnly))
            {
                QMessageBox::warning(
                            m_dialogParent, "Error",
                            QString("Could not create the temp file \"%1\" for bookmark \"%2\" (\"%3\").\nError: %4 %5")
                            .arg(mhtFilePathName, ib.title, ib.uri, QString::number(mhtfile.error()), mhtfile.errorString()));
                continue;
            }
            if (-1 == mhtfile.write(ib.ExPr_attachedFileData))
            {
                QMessageBox::warning(
                            m_dialogParent, "Error",
                            QString("Could not write data to the temp file \"%1\" for bookmark \"%2\" (\"%3\").\nError: %4 %5")
                            .arg(mhtFilePathName, ib.title, ib.uri, QString::number(mhtfile.error()), mhtfile.errorString()));
                mhtfile.close();
                continue;
            }
            if (!mhtfile.flush())
            {
                QMessageBox::warning(
                            m_dialogParent, "Error",
                            QString("Could not flush data to the temp file \"%1\" for bookmark \"%2\" (\"%3\").\nError: %4 %5")
                            .arg(mhtFilePathName, ib.title, ib.uri, QString::number(mhtfile.error()), mhtfile.errorString()));
                mhtfile.close();
                continue;
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
            bf.Ex_RemoveAfterAttach = true; //Will be immediately removed.

            bookmarkFiles.append(bf);
        }

        if (ib.Ex_status == ImportedBookmark::S_AnalyzedImportOK)
        {
            //[KeepDefaultFile-1].Generalization: Any bookmark MUST have a default file if it has files.
            //  (If it doesn't have files just pass -1, otherwise you may crash AddOrEditBookmark.)
            int defaultFileIndex = (bookmarkFiles.empty() ? -1 : 0);

            BookmarkManager::BookmarkData bdata;
            bdata.BID = -1; //Not important.
            bdata.Name = ib.title; //TODO: Title-less bookmarks possible? Don't let them happen. Set to url or sth.
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
                continue;
            }

            success = dbm->bms.UpdateBookmarkExtraInfos(
                        addedBID, QList<BookmarkManager::BookmarkExtraInfoData>(), ib.ExPr_ExtraInfosList);
            if (!success)
            {
                //A messagebox must have already been displayed.
                bbLogic.RollBackActionTransaction();
                continue;
            }

            bbLogic.CommitActionTransaction();

            //Success!
            addedBIDs.append(addedBID);
            allAssociatedTIDs.unite(QSet<long long>::fromList(associatedTIDs));
        }
        else
        {
            //TODO: Implement
            QMessageBox::warning(m_dialogParent, "Error",
                                 QString("Unknown bookmark import status %1 for imported bookmark \"%2\" (\"%3\").")
                                 .arg(QString::number((int)ib.Ex_status), ib.title, ib.uri));
            continue;
        }

        //Don't put a thing here. There are `continue`s in the code.
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
    return url.host() + '/' + url.path() + '#' + url.fragment();
}

QString BookmarkImporter::extraInfoField(const QString& fieldName, const QList<BookmarkManager::BookmarkExtraInfoData>& extraInfos)
{
    foreach (const BookmarkManager::BookmarkExtraInfoData& exInfo, extraInfos)
        if (exInfo.Name == fieldName)
            return exInfo.Value;
    return QString();
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
