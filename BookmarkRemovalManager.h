#pragma once
#include "ISubManager.h"
#include <QStringList>

class DatabaseManager;

class BookmarkRemovalManager : public ISubManager
{
    friend class DatabaseManager;

private:
    DatabaseManager* dbm;

public:
    BookmarkRemovalManager(QWidget* dialogParent, DatabaseManager* dbm, Config* conf);

    bool MoveBookmarkToTrash(long long BID);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
