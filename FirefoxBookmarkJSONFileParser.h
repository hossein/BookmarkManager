#pragma once
#include "IManager.h"
#include <QString>
#include <QList>
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

class FirefoxBookmarkJSONFileParser : public IManager
{
public:
    FirefoxBookmarkJSONFileParser(QWidget* dialogParent, Config* conf);

    bool ParseFile(const QString& jsonFilePath);

private:
    bool processObject(const QJsonObject& obj);

    /// We must be sure of `type` value before calling the following two functions.
    bool processBookmark(const QJsonObject& obj);
    bool processFolder(const QJsonObject& obj);

    bool processAnno(const QJsonObject& obj, int annoIndex, const QString& quickGuid,
                     QString& annoName, QString& annoValue);
};
