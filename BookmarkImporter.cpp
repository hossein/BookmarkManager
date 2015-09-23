#include "BookmarkImporter.h"

#include <QDebug>
#include <QUrl>

BookmarkImporter::BookmarkImporter(DatabaseManager* dbm, Config* conf)
    : dbm(dbm), conf(conf)
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
    //Note: Until e7d886fd2227165c67cd61e75137622ada874e4c @ 20141116 we queried `firefox-guid` and
    //      wanted to query other browsers' unique ids too; but it appears that guid's of firefox
    //      are not what their name suggests. There may be multiple bookmarks with the same url but
    //      different guids that are in different folders representing firefox bookmarks' tags, and
    //      not only guid but every property, including the adding and modify dates differs on them.
    //Also if unique ids match but urls dont then don't assume bookmarks are equal. So what was its
    //      usage after all? So it was removed.

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
                QString ffGuidField = extraInfoField("firefox-guid", extraInfos);
                detailsMatch = detailsMatch && (!ffGuidField.isNull() && ib.guid == bdata.Name);

                detailsMatch = detailsMatch && (ib.description == bdata.Desc);
                detailsMatch = detailsMatch && (ib.uri == bdata.URL);

                if (detailsMatch)
                    ib.Ex_status = ImportedBookmark::S_AnalyzedExactExistent;
                else
                    ib.Ex_status = ImportedBookmark::S_AnalyzedSimilarExistent;

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
    //TODO [IMPORT]
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
