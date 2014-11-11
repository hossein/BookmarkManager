#pragma once
#include "IManager.h"
#include "BookmarkImporters/ImportedEntity.h"

class QJsonObject;

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
