#pragma once
#include "IManager.h"
#include "BookmarkImporters/ImportedEntity.h"

#include <QList>

class QJsonObject;

class FirefoxBookmarkJSONFileParser : public IManager
{
public:
    FirefoxBookmarkJSONFileParser(QWidget* dialogParent, Config* conf);

    bool ParseFile(const QString& jsonFilePath, ImportedEntityList& elist);

private:
    bool processObject(const QJsonObject& obj, ImportedEntityList& elist);

    /// We must be sure of `type` value before calling the following two functions.
    bool processBookmark(const QJsonObject& obj, ImportedEntityList& elist);
    bool processFolder(const QJsonObject& obj, ImportedEntityList& elist);

    bool processAnno(const QJsonObject& obj, int annoIndex, const QString& quickGuid,
                     QString& annoName, QString& annoValue);
};
