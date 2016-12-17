#pragma once
#include "Database/ISubManager.h"
#include "Files/FileManager.h"
#include <QHash>
#include <QStringList>

class DatabaseManager;

/// Manages bookmark folders.
/// The special folder '0, Unsorted Bookmarks' is the parent of all other folders. Its parent is -1.
/// We do have a '-1, All Bookmarks' folder, but we decided not to touch the DB too much so as not to
///   have to create migrations. It was more intuitive that we set parent of all other folders to
///   this -1 folder and also save it in the database however. But it didn't reduce code complexity
///   too much. So it's just a fake folder inside the program, not in the DB. It's parent is also -1.
///
/// Relationship with FileArchives:
///   Each BookmarkFolder can specify its FileArchive so that the bookmarks in that folder store
///   their files in the file archive. However, there is the question of how folders' layout should
///   look like in each file archive. We have three choices:
///   A. Don't reflect BookmarkFolder layout on filesystem folders.
///   B. Reflect the full absolute path of BookmarkFolder on each file archive on the filesystem.
///   C. Reflect the relative path of BookmarkFolder on the file archive.
///   We don't want to choose A because it is unintuitive. We actually discontinued the use of the
///   'f/fi/file' prefixed layout in FAM because it was unintuitive and anyway nowadays OSes support
///   folders with lots of files in them.
///   The difference between B and C is, if we have a BookmarkFolder layout like this:
///     Root (arch0) --> F1 --> F2 --> F3 (arch1) --> F4
///   Then using layout B, arch1's directory layout will look like this:
///     C:\Arch1 (empty folder)\F1 (empty folder)\F2 (empty folder)\F3\{F4 folder and other Files}
///   But with layout C arch1's directory will not contain unnecessary parent dirs:
///     C:\Arch1\{F4 folder and other Files}
///   Layout C is the most intuitive layout. So we use it, but we have to solve some problems first.
///   Suppose we have the following BookmarkFolder layout:
///     Root (arch0) --> F1 --> F2 (arch1) --> F3 --> F4 (arch0) --> F5
///   Then in this case the directory of arch0 on the filesystem will contain both F1 and F5 as its
///   children, while in reality F5 is logically a grandparent of F1. To solve it we can disallow
///   setting a FileArchive on a BookmarkFolder when that same FileArchive was set on an ancestor
///   BookmarkFolder. With some thinking we can conclude that this avoids the mentioned problem.
///   But there is a more general problem.
///   Suppose this second BookmarkFolder layout, which does not cause the previous problem:
///     Root (arch0) --+-> F1 --> F2 (arch1) --> FN
///                    \--> F4 --> F5 (arch1) --> FN
///   Then arch1 will contain two separate folders, maybe with separate functions, and maybe with
///   the same name. The final solution that comes to mind to prevent all of these issues is:
///   We let each FileArchive be specified as the FileArchive OF AT MOST ONE BookmarkFolder.
///   The mentioned problems will not happen then. However there is the problem that the user
///   can specify the same filesystem folder for two or more file archives but that's another story
///   and will be prevented in FileArchiveManager.
//TODO: Do we currently prevent creating folders having the same archive as Unsorted thingie?
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
        /// Ex_AbsolutePath is READ-ONLY.
        /// It must not be used to get folderHint for a file inside a fileArchive, since some
        /// folders can have separate file archives. Use `GetFileArchiveAndFolderHint()` instead.
        QString Ex_AbsolutePath;
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
    /// Finds the file archive for the folder, which may be inherited from its parent.
    bool GetFileArchiveAndFolderHint(long long FOID, QString& fileArchiveName, QString& folderHint);

    /// The following two functions return a list of first-level sub folders.
    QList<long long> GetChildrenIDs(long long FOID);
    QStringList GetChildrenNames(long long FOID);

    /// Returns the absolute path for normal folders, or folder name for special folders.
    QString GetPathOrName(long long FOID);

private:
    bool CalculateAbsolutePaths();

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModelsAndInternalTables();
};
