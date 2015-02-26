#pragma once
#include "BookmarkImporters/ImportedEntity.h"
#include "DatabaseManager.h"

#include <QString>
#include <QHash>
#include <QMultiHash>
#include <QList>

/// This class first needs to initialized, then it should analyze the to-be-imported bookmarks
///     before really importing them. It should be re-initialized each time an import is going to
///     happen so that it can collect the bookmark url list again.
/// Analyzing checks for existing urls. If a url exists with exact same anchors case-sensitively,
///     there is an 'almost exact' match for an existing bookmark. We don't further check the urls.
///     If text cases differs or one of the bookmarks refers to another anchor in the same file,
///     there is a 'similar' match. In the mentioned comparison, protocols (http/https/ftp),
///     user info (john:doe) and ports (:8080) are discarded.
/// Analyzer sets the corresponding 'Ex_' fields in the bookmarks list.
///     User should make the changes he wants and decide what to do for duplicate/similar
///     bookmarks that are going to be imported, then call the Import function.
/// Note: Some of the folders and bookmarks that this analyzes are still without titles. Some of
///     these may include duplicate urls which neither one had titles, but some of the bookmarks
///     that are not tag-duplicates of another bookmarks may have empty titles.
class BookmarkImporter
{
private:
    DatabaseManager* dbm;
    Config* conf;
    ImportedEntityList::ImportSource importSource;
    QMultiHash<QString, long long> existentBookmarksForUrl;
    QMap<int, int> folderItemsIndexInArray;

public:
    BookmarkImporter(DatabaseManager* dbm, Config* conf);

    bool Initialize(ImportedEntityList::ImportSource importSource);
    bool Analyze(ImportedEntityList& elist);
    bool Import(ImportedEntityList& elist);

private:
    QString GetURLForFastComparison(const QString& originalUrl);
    QString GetURLForAlmostExactComparison(const QString& originalUrl);
    /// Returns a null QString if extra infos don't contain the field.
    QString extraInfoField(const QString& fieldName, const QList<BookmarkManager::BookmarkExtraInfoData>& extraInfos);

    QString bookmarkTagAccordingToParentFolders(ImportedEntityList& elist, int bookmarkIndex);

};
