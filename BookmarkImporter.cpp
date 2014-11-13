#include "BookmarkImporter.h"

#include <QUrl>

BookmarkImporter::BookmarkImporter(DatabaseManager* dbm, Config* conf)
    : dbm(dbm), conf(conf)
{

}

bool BookmarkImporter::Initialize()
{
    QHash<long long, QString> bookmarkURLs;
    bool success = dbm->bms.RetrieveAllFullURLs(bookmarkURLs);
    if (!success)
        return false;

    existentBookmarksForUrl.clear();
    for (auto it = bookmarkURLs.cbegin(); it != bookmarkURLs.cend(); ++it)
        existentBookmarksForUrl.insertMulti(GetURLForFastComparison(it.value()), it.key());

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
    return url.host() + '/' + url.path();
}
