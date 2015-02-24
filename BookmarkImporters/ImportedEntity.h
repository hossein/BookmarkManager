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
};

struct ImportedBookmarkFolder
{
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
    QStringList additionalTags;
};

struct ImportedEntityList
{
    QList<ImportedBookmark> iblist;
    QList<ImportedBookmarkFolder> ibflist;
};
