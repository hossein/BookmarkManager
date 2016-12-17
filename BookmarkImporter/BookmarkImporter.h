#pragma once
#include "BookmarkImporter/ImportedEntity.h"
#include "Database/DatabaseManager.h"

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
    QWidget* m_dialogParent;
    QMultiHash<QString, long long> existentBookmarksForUrl;
    QMap<int, int> folderItemsIndexInArray;

    QList<long long> m_addedBIDs;
    QSet<long long> m_allAssociatedTIDs;
    QSet<ImportedBookmark*> m_failedProcessOrImports;
    QString m_tempPath;

public:
    BookmarkImporter(DatabaseManager* dbm, QWidget* dialogParent);

    //Initial init and analyzing functions.
    bool Initialize();
    bool Analyze(ImportedEntityList& elist);

    //Cumulative import function. Doesn't mark anything as failed.
    bool Import(ImportedEntityList& elist, QList<long long>& addedBIDs,
                QSet<long long>& allAssociatedTIDs,
                QList<ImportedBookmark*>& failedProcessOrImports);

    //Controlled one-by-one import functions. Use these.
    //Note: Marking an import as failed does not mean the bookmark was not imported. Maybe only its
    //  file saving was not successful. Calling MarkAsFailed twice on the same bookmark is safe.
    bool InitializeImport();
    bool ImportOne(const ImportedBookmark& ib, long long importFOID);
    void MarkAsFailed(ImportedBookmark* ib);
    void FinalizeImport(QList<long long>& addedBIDs, QSet<long long>& allAssociatedTIDs,
                        QList<ImportedBookmark*>& failedProcessOrImports);

private:
    QString bookmarkTagAccordingToParentFolders(ImportedEntityList& elist, int bookmarkIndex);

    /// Both `title` and `desc` will be empty if there is no bracketed description in the title.
    /// Don't check for emptyness of just one of them, it might simply be an empty title with a
    ///   non-empty description, or vice versa!
    void BreakTitleBracketedDescription(const QString& titleDesc, QString& title, QString& desc);
    /// Terminology: A 'duplicate' bookmark can be 'similar' or 'exact' duplicate of the imported bm.
    bool FindDuplicate(const ImportedBookmark& ib, const QList<long long>& almostDuplicateBIDs,
                       bool& foundSimilar, bool& foundExact, long long& duplicateBID);
    QString GetURLForFastComparison(const QString& originalUrl);
    QString GetURLForAlmostExactComparison(const QString& originalUrl);
    /// Returns a null QString if extra infos don't contain the field.
    QString extraInfoField(const QString& fieldName, const QList<BookmarkManager::BookmarkExtraInfoData>& extraInfos);

    void RemoveTempFileIfExists(const QString& filePathName);
};
