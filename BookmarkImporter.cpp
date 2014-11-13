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
