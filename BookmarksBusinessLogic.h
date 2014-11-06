#pragma once
#include <QString>

class Config;
class DatabaseManager;

class BookmarksBusinessLogic
{
private:
    DatabaseManager* dbm;
    Config* conf;

public:
    BookmarksBusinessLogic(DatabaseManager* dbm, Config* conf);

    bool DeleteBookmark(long long BID);

private:
    bool DoRollBackAction();
};
