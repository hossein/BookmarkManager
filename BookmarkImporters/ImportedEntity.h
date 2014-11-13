#pragma once
#include <QList>
#include <QString>
#include <QDateTime>

class QJsonObject;

struct ImportedBookmark
{
    ImportedBookmark()
    {
        this->Ex_status = S_NotAnalyzed;
    }

    QString title;
    QString guid;
    QString description;
    QString uri;
    QString charset;

    QString intId;
    QString intIndex;
    QString parentId;
    QDateTime dtAdded;
    QDateTime dtModified;

    //Managed by BookmarkImporter
    enum ImportedBookmarkStatus
    {
        S_NotAnalyzed = 0,
        S_ImportOK,
        S_DontImport,
        S_ReplaceExisting,
        S_AppendToExisting,
    };
    ImportedBookmarkStatus Ex_status;
    QStringList Ex_additionalTags;
};

struct ImportedBookmarkFolder
{
    QString title;
    QString guid;
    QString description;
    QString root;

    QString intId;
    QString intIndex;
    QString parentId;
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
