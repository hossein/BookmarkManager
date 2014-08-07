#pragma once
#include "ISubManager.h"
#include <QStringList>
#include <QtSql/QSqlQueryModel>

class DatabaseManager;

/// Tag management is case-insensitive. The first time you use a tag, it defines its name's
/// case style.
class TagManager : public ISubManager
{
    friend class DatabaseManager;

public:
    QSqlQueryModel model;

    struct
    {
        int TID;
        int TagName;
    } tidx;

public:
    TagManager(QWidget* dialogParent, Config* conf);

    bool RetrieveBookmarkTags(long long BID, QStringList& tagsList);
    /// It's okay if tags are duplicate.
    /// Puts only those tags who were added to the bookmark in associatedTIDs, whether tags were
    /// themselves new or not. E.g if a bookmark is tagged A B C and we call this function with
    /// tags B C D, it just puts D in the associatedTIDs, whether or not a tag called D already
    /// exists or not.
    bool SetBookmarkTags(long long BID, const QStringList& tagsList, QList<long long>& associatedTIDs);

private:
    long long MaybeCreateTagAndReturnTID(const QString& tagName);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
