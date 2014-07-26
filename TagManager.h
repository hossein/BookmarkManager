#pragma once
#include "ISubManager.h"
#include <QStringList>
#include <QtSql/QSqlQueryModel>

class DatabaseManager;

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

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
