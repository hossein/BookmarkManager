#pragma once
#include <QList>
#include <QString>
#include <QDateTime>

class QJsonObject;

struct ImportedBookmark
{
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
};

struct ImportedEntityList
{
    QList<ImportedBookmark> iblist;
    QList<ImportedBookmarkFolder> ibflist;
};
