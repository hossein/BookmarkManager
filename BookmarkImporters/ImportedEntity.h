#pragma once
#include <QList>
#include <QString>
#include <QStringList>
#include <QDateTime>

class QJsonObject;

struct ImportedBookmark
{
    ImportedBookmark()
    {
        this->Ex_import = true;
        this->Ex_status = S_NotAnalyzed;
    }

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

    //Managed by BookmarkImporter
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
    QStringList Ex_additionalTags;
    bool Ex_finalImport;
    QStringList Ex_finalTags;
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
    QStringList Ex_finalTags;
};

struct ImportedEntityList
{
    enum ImportSource { Source_Firefox };
    ImportSource importSource;

    QList<ImportedBookmark> iblist;
    QList<ImportedBookmarkFolder> ibflist;
};
