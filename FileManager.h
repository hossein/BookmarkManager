#pragma once
#include "ISubManager.h"
#include <QtSql/QSqlQueryModel>

class DatabaseManager;

class FileManager : public ISubManager
{
    friend class DatabaseManager;

public:
    struct BookmarkFileIndexes
    {
        int BFID;
        int BID;
        int FID;
        int OriginalName;
        int ArchiveURL;
        int ModifyDate;
        int Size;
        int MD5;
    } bfidx;

public:
    FileManager(QWidget* dialogParent, Config* conf);
    ~FileManager();

    bool InitializeFilesDirectory();

    bool RetrieveBookmarkFilesModel(long long BID, QSqlQueryModel& filesModel);

public:
    //TODO: Private?
    /// Modifies the `path` and puts the FileArchive address in it.
    bool PutInsideFileArchive(QString& filePathName, bool removeOriginalFile);
    bool IsInsideFileArchive(const QString& path);

private:
    int SumOfFileNameLetters(const QString& fileNameOnly);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
