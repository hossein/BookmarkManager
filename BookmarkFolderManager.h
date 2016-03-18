#pragma once
#include "ISubManager.h"
#include "FileManager.h"
#include <QHash>
#include <QStringList>

class DatabaseManager;

/// Manages bookmark folders. There is a folder '0, Unsorted Bookmarks' that is the parent of all
///   other folders. Its parent is -1.
class BookmarkFolderManager : public ISubManager
{
    friend class DatabaseManager;

public:
    struct BookmarkFolderIndexes
    {
        int FOID;
        int ParentFOID;
        int Name;
        int Desc;
        int DefFileArchive;
    } foidx;

    struct BookmarkFolderData
    {
        long long FOID;
        long long ParentFOID;
        QString Name;
        QString Desc;
        QString DefFileArchive;

        //This information are not saved in the database.
        QString Ex_AbsolutePath; //ReadOnly
    };

public:
    ///Everybody can use this JUST FOR READING. For modifying one must call this class's functions.
    ///This is in sync with the database.
    QHash<long long, BookmarkFolderData> bookmarkFolders;

public:
    BookmarkFolderManager(QWidget* dialogParent, Config* conf);

    bool RetrieveBookmarkFolder(long long FOID, BookmarkFolderData& fodata);
    /// For adding bookmark folder (i.e when FOID == -1), both the FOID arg and fodata.FOID will contain
    ///   the FOID of the inserted folder.
    bool AddOrEditBookmarkFolder(long long& FOID, BookmarkFolderData& fodata);
    /// Important: Before removing, the folder must not have any sub-folders or bookmarks in it,
    ///   otherwise we encounter a foreign key constraint violation.
    bool RemoveBookmarkFolder(long long FOID);

    //Helper functions
    /// The following two functions return a list of first-level sub folders.
    QList<long long> GetChildrenIDs(long long FOID);
    QStringList GetChildrenNames(long long FOID);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModelsAndInternalTables();
};
