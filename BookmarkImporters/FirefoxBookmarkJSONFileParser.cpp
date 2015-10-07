#include "FirefoxBookmarkJSONFileParser.h"

#include "Util.h"

#include <QFile>
#include <QFileInfo>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

FirefoxBookmarkJSONFileParser::FirefoxBookmarkJSONFileParser(QWidget* dialogParent, Config* conf)
    : IManager(dialogParent, conf)
{

}

bool FirefoxBookmarkJSONFileParser::ParseFile(const QString& jsonFilePath, ImportedEntityList& elist)
{
    QFile jsonFile(jsonFilePath);
    if (!jsonFile.exists())
        return Error("The specified file does not exist:\n" + jsonFilePath);

    if (!jsonFile.open(QIODevice::ReadOnly))
        return Error("Could not open the specified file:\n" + jsonFilePath + "\nThe error is:" +
                     jsonFile.errorString());

    QByteArray jsonByteArray = jsonFile.readAll();
    jsonFile.close();

    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonByteArray, &jsonParseError);

    if (jsonParseError.error != QJsonParseError::NoError)
    {
        const int offset = jsonParseError.offset;
        int line, col;
        Util::FindLineColumnForOffset(jsonByteArray, offset, line, col);
        return Error("Error while parsing JSON: " + jsonParseError.errorString() +
                     QString("\nOffset %1, Line %2, Column %3").arg(offset).arg(line).arg(col));
    }

    if (!doc.isObject())
        return Error("File format error: Root element is not an object.");

    //Do it for caller.
    elist.iblist.clear();
    elist.ibflist.clear();

    bool success = processObject(doc.object(), elist);
    return success;
}

bool FirefoxBookmarkJSONFileParser::processObject(const QJsonObject& obj, ImportedEntityList& elist)
{
    if (!obj.keys().contains("type"))
        return Error("File format error: Object does not have a 'type' key.");

    QJsonValue typeValue = obj["type"];
    if (!typeValue.isString())
        return Error("File format error: Object type is not a string.");

    QString type = typeValue.toString();

    if (type == "text/x-moz-place")
    {
        return processBookmark(obj, elist);
    }
    else if (type == "text/x-moz-place-container")
    {
        return processFolder(obj, elist);
    }
    else if (type == "text/x-moz-place-separator")
    {
        //Don't process it.
    }
    else
    {
        return Error(QString("File format error: Unknown object type '%1'.").arg(type));
    }

    return true;
}

#define CHECK_AND_STORE_INTEGER(VARNAME,KEY) \
    value = obj[KEY]; \
    if (!value.isDouble()) \
        return Error(QString("Error in bookmark %1: Value of %2 field is not a number.") \
                     .arg(quickId, KEY)); \
    VARNAME = value.toInt();

#define CHECK_AND_STORE_STRING(VARNAME,KEY) \
    value = obj[KEY]; \
    if (!value.isString()) \
        return Error(QString("Error in bookmark %1: Value of %2 field is not a string.") \
                     .arg(quickId, KEY)); \
    VARNAME = value.toString();

#define CHECK_AND_STORE_DTSTAMP(VARNAME,KEY) \
    value = obj[KEY]; \
    if (!value.isDouble()) \
        return Error(QString("Error in bookmark %1: Value of %2 field is not a number/date.") \
                     .arg(quickId, KEY)); \
    /* We avoid integer overflow; we don't use value.toInt(); should use this way. */ \
    VARNAME = QDateTime::fromMSecsSinceEpoch(static_cast<long long>(value.toDouble()));

#define STORE_IF_EXISTS_INTEGER(VARNAME, KEY, DEFVALUE) \
    if (keys.contains(KEY)) { CHECK_AND_STORE_INTEGER(VARNAME,KEY); } else VARNAME = DEFVALUE;

#define STORE_IF_EXISTS_STRING(VARNAME, KEY, DEFVALUE) \
    if (keys.contains(KEY)) { CHECK_AND_STORE_STRING (VARNAME,KEY); } else VARNAME = DEFVALUE;

#define STORE_IF_EXISTS_DTSTAMP(VARNAME, KEY, DEFVALUE) \
    if (keys.contains(KEY)) { CHECK_AND_STORE_DTSTAMP(VARNAME,KEY); } else VARNAME = DEFVALUE;


bool FirefoxBookmarkJSONFileParser::processBookmark(const QJsonObject& obj, ImportedEntityList& elist)
{
    QStringList requiredBookmarkKeys, otherBookmarkKeys;
    requiredBookmarkKeys << "title" << "id" << "parent" << "type" << "uri";
    otherBookmarkKeys << "index" << "guid" << "dateAdded" << "lastModified" << "annos" << "charset";

    //Case sensitive compare
    QStringList keys = obj.keys();
    UtilT::ListDifference(requiredBookmarkKeys, keys);
    UtilT::ListDifference(otherBookmarkKeys, keys); //To remove otherKeys entries from keys.

    QString quickId = obj["id"].toString(); /* may not be empty */
    if (requiredBookmarkKeys.length() > 0)
        return Error(QString("Required attributes don't exist for bookmark %1: %2")
                     .arg(quickId, requiredBookmarkKeys.join(", ")));

    if (keys.length() > 0)
        //We don't expect other names; but they may appear in future firefox versions.
        qDebug() << QString("Extra unknown attributes for bookmark %1: %2")
                    .arg(quickId, keys.join(", "));

    QJsonValue value;
    keys = obj.keys(); //Required for the macros.

    ImportedBookmark ib;
    CHECK_AND_STORE_STRING (ib.title     , "title"       );
    CHECK_AND_STORE_INTEGER(ib.intId     , "id"          );
    CHECK_AND_STORE_INTEGER(ib.parentId  , "parent"      );
    CHECK_AND_STORE_STRING (ib.uri       , "uri"         );

    STORE_IF_EXISTS_STRING (ib.charset   , "charset"     , QString());
    STORE_IF_EXISTS_STRING (ib.guid      , "guid"        , QString());
    STORE_IF_EXISTS_INTEGER(ib.intIndex  , "index"       , -1);
    STORE_IF_EXISTS_DTSTAMP(ib.dtAdded   , "dateAdded"   , QDateTime());
    STORE_IF_EXISTS_DTSTAMP(ib.dtModified, "lastModified", QDateTime());

    if (keys.contains("annos"))
    {
        if (!obj["annos"].isArray())
            return Error(QString("'Annos' for bookmark with id %1 is not an array.").arg(quickId));

        QString annoName, annoValue;
        QJsonArray annos = obj["annos"].toArray();
        for (int i = 0; i < annos.size(); i++)
        {
            if (!annos[i].isObject())
                return Error(QString("An 'annos' entry for bookmark with id %1 is not an object.").arg(quickId));

            const QJsonObject anno = annos[i].toObject();
            if (!processAnno(anno, i, quickId, annoName, annoValue))
                return false;

            if (annoName == "bookmarkProperties/description")
            {
                ib.description = annoValue;
            }
            else if (annoName == "Places/SmartBookmark")
            {
                //Discard this.
            }
            else
            {
                qDebug() << QString("Unknown annos[%1] for bookmark with id %2: %3 = %4")
                            .arg(QString::number(i), quickId, annoName, annoValue);
            }
        }
    }

    //Don't import firefox's special bookmarks
    const QString uriLeft = ib.uri.left(6);
    if (uriLeft == "place:" || uriLeft == "about:")
        return true;

    elist.iblist.append(ib);

    //qDebug() << "Imported bookmark " << ib.title;
    return true;
}

bool FirefoxBookmarkJSONFileParser::processFolder(const QJsonObject& obj, ImportedEntityList& elist)
{
    QStringList requiredFolderKeys, otherFolderKeys;
    requiredFolderKeys << "title" << "id" << "type" << "children";
    otherFolderKeys << "index" << "guid" << "parent" << "dateAdded" << "lastModified" << "annos"
                    << "root" << "livemark";

    //Case sensitive compare
    QStringList keys = obj.keys();
    UtilT::ListDifference(requiredFolderKeys, keys);
    UtilT::ListDifference(otherFolderKeys, keys); //To remove otherKeys entries from keys.

    QString quickId = obj["id"].toString(); /* may not be empty */
    if (requiredFolderKeys.length() > 0)
        return Error(QString("Required attributes don't exist for folder with id %1: %2")
                     .arg(quickId, requiredFolderKeys.join(", ")));

    if (keys.length() > 0)
        //We don't expect other names; but they may appear in future firefox versions.
        qDebug() << QString("Extra unknown attributes for folder with id %1: %2")
                    .arg(quickId, keys.join(", "));

    QJsonValue value;
    keys = obj.keys(); //Required for the macros.

    ImportedBookmarkFolder ibf;
    CHECK_AND_STORE_STRING (ibf.title     , "title"       );
    CHECK_AND_STORE_INTEGER(ibf.intId     , "id"          );

    STORE_IF_EXISTS_STRING (ibf.root      , "root"        , QString());
    STORE_IF_EXISTS_STRING (ibf.guid      , "guid"        , QString());
    STORE_IF_EXISTS_INTEGER(ibf.intIndex  , "index"       , -1);
    STORE_IF_EXISTS_INTEGER(ibf.parentId  , "parent"      , -1);
    STORE_IF_EXISTS_DTSTAMP(ibf.dtAdded   , "dateAdded"   , QDateTime());
    STORE_IF_EXISTS_DTSTAMP(ibf.dtModified, "lastModified", QDateTime());

    if (keys.contains("annos"))
    {
        if (!obj["annos"].isArray())
            return Error(QString("'Annos' for folder with id %1 is not an array.").arg(quickId));

        QString annoName, annoValue;
        QJsonArray annos = obj["annos"].toArray();
        for (int i = 0; i < annos.size(); i++)
        {
            if (!annos[i].isObject())
                return Error(QString("annos[%1] entry for folder with id %2 is not an object.")
                             .arg(QString::number(i), quickId));

            const QJsonObject anno = annos[i].toObject();
            if (!processAnno(anno, i, quickId, annoName, annoValue))
                return false;

            if (annoName == "bookmarkProperties/description")
            {
                ibf.description = annoValue;
            }
            else if (annoName == "bookmarkPropertiesDialog/folderLastUsed" ||
                     annoName == "placesInternal/READ_ONLY" ||
                     annoName == "livemark/feedURI" || annoName == "livemark/siteURI")
            {
                //Discard this.
            }
            else
            {
                qDebug() << QString("Unknown annos[%1] for folder with id %2: %3 = %4")
                            .arg(QString::number(i), quickId, annoName, annoValue);
            }
        }
    }

    elist.ibflist.append(ibf);

    if (!obj["children"].isArray())
        return Error(QString("Children attribute for folder with id %1 is not an array.").arg(quickId));

    QJsonArray childrenArray = obj["children"].toArray();
    for (int i = 0; i < childrenArray.size(); i++)
    {
        if (!childrenArray[i].isObject())
            return Error(QString("children[%1] entry for folder with id %2 is not an object.")
                         .arg(QString::number(i), quickId));

        const QJsonObject child = childrenArray[i].toObject();
        if (!processObject(child, elist))
            return false;
    }

    //qDebug() << "Encountered folder " << ibf.title;
    return true;
}

bool FirefoxBookmarkJSONFileParser::processAnno(const QJsonObject& obj, int annoIndex, const QString& quickId,
                                                QString& annoName, QString& annoValue)
{
    QStringList requiredAnnosKeys, otherAnnosKeys;
    requiredAnnosKeys << "name"  << "value";
    otherAnnosKeys    << "flags" << "expires" << "mimeType" << "type";

    //Case sensitive compare.
    QStringList keys = obj.keys();
    UtilT::ListDifference(requiredAnnosKeys, keys);
    UtilT::ListDifference(otherAnnosKeys, keys);

    if (requiredAnnosKeys.length() > 0)
        return Error(QString("Required attributes don't exist for annos[%1] of bookmark/folder with id %2: %3")
                     .arg(QString::number(annoIndex), quickId, requiredAnnosKeys.join(", ")));

    if (keys.length() > 0)
        //Maybe extra attributes appear in future firefox versions.
        qDebug() << QString("Extra unknown attributes for annos[%1] of bookmark/folder with id %2: %3")
                    .arg(QString::number(annoIndex), quickId, keys.join(", "));

    QJsonValue value;
    CHECK_AND_STORE_STRING(annoName , "name" );

    value = obj["value"];
    if (value.isDouble())
        annoValue = QString::number(value.toDouble(), 'g', 20);
    else
        annoValue = value.toString();

    return true;
}
