#pragma once
#include "IManager.h"
#include "BookmarkImporters/ImportedEntity.h"

#include <QList>

class QJsonObject;

/// IMPORTANT: The ARRAY INDEX of the child folders that this class generates is always BIGGER THAN
///   the array index of their parent folders. This is CRUCIAL for working of the other parts, which
///   ALWAYS simply use NESTED `for` loops for parent-child folder operations instead of recursive
///   functions.
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
