#include "BookmarkImporter.h"

#include <QUrl>

BookmarkImporter::BookmarkImporter(DatabaseManager* dbm, Config* conf)
    : dbm(dbm), conf(conf)
{

}

bool BookmarkImporter::Initialize(ImportSource importSource)
{
    this->importSource = importSource;

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
    //      different guids that are in different folders representing firefox bookmarks' tags.
    //Also if unique ids match but urls dont then don't assume bookmarks are equal. So what was its
    //      usage after all? So it was removed.

    return true;
}

bool BookmarkImporter::Analyze(ImportedEntityList& elist)
{
    for (int i = 0; i < elist.iblist.size(); i++)
    //foreach (ImportedBookmark& ib, elist.iblist)
    {
        //By reference.
        ImportedBookmark& ib = elist.iblist[i];

        //Check the urls for duplicates among existing bookmarks.
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
            }
        }

        //If urls are not similar
        ib.Ex_status = ImportedBookmark::S_AnalyzedImportOK;
    }

    return true;
}

bool BookmarkImporter::Import(ImportedEntityList& elist)
{
    return true;
}

QString BookmarkImporter::GetURLForFastComparison(const QString& originalUrl)
{
    //TODO: Check both these functions this with file: and mailto: urls.
    QUrl url(originalUrl);
    return QString(url.host() + '/' + url.path()).toLower();
}

QString BookmarkImporter::GetURLForAlmostExactComparison(const QString& originalUrl)
{
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
