#include "BookmarkImporter.h"

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
    //NOTE: In case of other importers, they should be queried too.
    QList<BookmarkManager::BookmarkExtraInfoData> extraInfos;
    success = dbm->bms.RetrieveSpecificExtraInfoForAllBookmarks("firefox-guid", extraInfos);
    if (!success)
        return false;

    existentBookmarksForUniqueId.clear();
    foreach (const BookmarkManager::BookmarkExtraInfoData& exInfo, extraInfos)
        if (exInfo.Type == BookmarkManager::BookmarkExtraInfoData::Type_Text)
            existentBookmarksForUniqueId.insertMulti(exInfo.Value, exInfo.BID);

    //TODO: If unique ids match but urls dont then don't assume bookmarks are equal.

    return true;
}

bool BookmarkImporter::Analyze(ImportedEntityList& elist)
{
    foreach (ImportedBookmark& ib, elist.iblist)
    {
        //First check the unique ids for duplicates.
        if (existentBookmarksForUniqueId.contains(ib.guid))
        {
            //Get the other bookmark(s). Compare with their url, if url didn't match, dismiss this.
            //  If urls match, well it will be caught by the next if check.
            //So TODO: Why we bother checking guids at all?
        }

        //Whether a guid found or not, also check the url for duplicates.
        QString fastDuplCheckURL = GetURLForFastComparison(ib.uri);
        if (existentBookmarksForUrl.contains(fastDuplCheckURL))
        {
            //Whoops, we found a similar match until now.
        }


    }
}

bool BookmarkImporter::Import(ImportedEntityList& elist)
{

}

QString BookmarkImporter::GetURLForFastComparison(const QString& originalUrl)
{
    //TODO: Check this with file: and mailto: urls.
    QUrl url(originalUrl);
    return QString(url.host() + '/' + url.path()).toLower();
}
