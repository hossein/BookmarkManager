#pragma once
#include <QList>
#include <QString>
#include <QStringList>
#include <QDateTime>

#include "Bookmarks/BookmarkManager.h"

class QJsonObject;

struct ImportedBookmark
{
    ImportedBookmark()
    {
        this->Ex_import = true;
        this->Ex_status = S_NotAnalyzed;
    }

    //== Data directly from import ============================================
    //  Inserted by e.g FirefoxBookmarkJSONFileParser

    QString title;
    QString guid;
    QString description;
    QString uri;
    QString charset;

    int intId;
    int intIndex;
    int parentId;
    QDateTime dtAdded;
    QDateTime dtModified;

    //== Metadata about the to-be-imported bookmark before initializing =======
    QString ExMd_importedFilePath;

    //== Managed by initial analyzing =========================================
    //  From by BookmarkImporter::Analyze and ImportedBookmarksPreviewDialog

    enum ImportedBookmarkStatus
    {
        //Before analysis
        S_NotAnalyzed = 0,
        //After analysis
        S_AnalyzedExactExistent,
        S_AnalyzedSimilarExistent,
        S_AnalyzedImportOK,
        //Decisions of user
        S_ReplaceExisting,
        S_AppendToExisting,
    };
    bool Ex_import;
    ImportedBookmarkStatus Ex_status;
    QList<long long> Ex_AlmostDuplicateExistentBIDs;
    long long Ex_DuplicateExistentComparedBID;
    QStringList Ex_additionalTags;
    bool Ex_finalImport;
    QStringList Ex_finalTags;

    //== After processing =====================================================
    //  By ImportedBookmarksProcessor

    QString ExPr_attachedFileError; //Shows error if not empty
    QString ExPr_attachedFileName;
    QByteArray ExPr_attachedFileData;

    QList<BookmarkManager::BookmarkExtraInfoData> ExPr_ExtraInfosList;

    //== After importing (by the processor) ===================================
    //This is the final error that should be read anywhere.
    QString ExIm_finalError;
};

struct ImportedBookmarkFolder
{
    ImportedBookmarkFolder()
    {
        this->Ex_importBookmarks = true;
    }

    QString title;
    QString guid;
    QString description;
    QString root;

    int intId;
    int intIndex;
    int parentId;
    QDateTime dtAdded;
    QDateTime dtModified;

    //Managed by BookmarkImporter
    bool Ex_importBookmarks;
    QStringList Ex_additionalTags;
    QStringList Ex_semiFinalTags;
    QStringList Ex_finalTags;
};

struct ImportedEntityList
{
    ImportedEntityList()
    {
        removeImportedFiles = false;
    }

    long long importFOID;

    enum ImportSource { Source_Urls, Source_Files, Source_Firefox };
    ImportSource importSource;
    QString importSourceProfile;
    QString importSourceFileName;
    bool removeImportedFiles;

    QList<ImportedBookmark> iblist;
    QList<ImportedBookmarkFolder> ibflist;
};
