#pragma once
#include <QDebug>
#include <QString>

//Note: [Clear selection on useless right-click]
//Qt automatically selects the item under mouse, but if an item is already selected, it doesn't
//  deselect it when an item-less area is clicked! So we deselect it automatically. However
//  this also disables the BOTH-CLICK action in Win7 for displaying the context menu; we do
//  nothing about it.

//Note: [Why we don't delete file after app]
//  http://www.stackoverflow.com/questions/19734258/
//  how-do-i-wait-to-delete-a-file-until-after-the-program-i-started-has-finished-us
//When copying a file to sandbox and opening it, we don't wait for the started app to finish
//  and then delete the file. The reason is that this is difficult to do cross-platform and even
//  on Windows we can't always understand when the application is really closed or whether it has
//  spawned another app.
//  As a CONSEQUENCE, we can't show the 'File X was modified. Apply the modifications in FileArchive
//  too?' messages which may make the program more usable, and use the
//  'Optionally Editable Open/Direct Open' terminology (and logic) instead of
//  'Sandboxed Open/Editable Open' idea.
//So we delete the files later:
//- Deleting on closing BookmarkManager: Maybe best option, but user may lose modified sanboxed
//  files so we don't do it.
//- Deleting 30 seconds after startup: Maybe, but user may have again opened a file; that new file
//  gets deleted too.
//- Deleting on start-up: Slows down startup... but not too much! So we do it.
//  Note: And this method is another reason we need to allow only one app instance (as well as
//  because of database file usage).
//Other methods such as renaming the sandbox folder on start-up and remove it 30 seconds later, or
//  keep the list of newly opened files and don't delete them in the 30 seconds later clean up is
//  also possible; but we don't do it for now.

/// TODO: Plan:
///   x File Manager
///   x Make the changes that we need to require multiple file attaching and do it.
///   x Manage tag saving as comma separated integers/string or a separate table? (Done: the latter)
///   - Show files information on double click in BOTH add/edit and view bookmark dialogs.
///   x When user opens a file, CAN HE CHANGE IT? And should we save Changes? YES AND ALSO CHANGES
///     MUST BE SAVED IN THE DB TOO! Also, user should be warned against "changing changes multiple
///     files too". Add this last one as a note when you implemented file changing before multiple
///     file sharing.
///   x Save the selection/scroll position on add/edit/delete.
///   x View bookmark dialog
///   x In MainWindow, sort BMs by their adding date.
///   - Context menu which shows a brief info and has options edit/view/show in browser, etc etc
///     (much like how Word 2010 allows changing font and formatting as well as context-menu via
///     right-click)
///   x Filter by tags
///   x "Delete" moves the bookmarks or archivefiles to a "Trash", never deletes them!
///   x Search
///   x Bookmark extra info table.
///   - Import Firefox bookmarks (by both browsing the json file and selecting profile!)
///   x Linking Bookmarks to each other.
///   x Extension Association Edit Dialog: For each extension, show the list of Open With (but not
///     the system default Open dialog, because we want to be system-independent and not use
///     Windows' registry CLASSES, etc) and let user (only!) Remove the apps he needs. Also a
///     dialog for the global programs that are used for every extension. Done in OpenWithDialog.
///
/// LATER:
///   - Show %'ed urls, e.g Persian wikipedia urls in their decoded forms everywhere, including
///     import preview dialog and bookmark view dialog.
///   - Changes to Edit/Add UI for editing tags with mouse only
///   - Make the add/edit dialog non-modal, and allow editing/viewing more than one bookmark at once
///     (but do NOT allow opening the edit/view dialog for the same bookmark twice!)
///   - Full-text index for the words inside the saved web pages!
///   - Saving FileArchive as zip files, and dynamic number of files per separate-directories.
///   - "Trash" viewer and manager. (Implemented for files, should implement for tags and bms
///     (for bms only the trash table is created, trash handling is NOT implemented). IMPORTANT:
///     AND BE CAREFUL about restoring bookmarks from recycle bin, they might refer to BOTH
///     deleted Tags and Files!
///   - Mass Apply Tag, AND Mass Remove Tag! Yes!
///   - Zip every 30 backups of the db and MOVE them inside a directory OTHER THAN Backups.
///   - Delete file from RecycleBin when roolling back TransactFileOps:
///     (FULL CODE, HAHA!) http://social.msdn.microsoft.com/Forums/vstudio/en-US/
///                        05f1476f-a101-4766-847b-0bdf4f6ad397/restore-undelete-
///                        file-from-recycle-bin?forum=csharpgeneral
///     (ANOTHER PROJECT) http://www.codeproject.com/Articles/2783/
///                       How-to-programmatically-use-the-Recycle-Bin
///   - Sharing files between two or multiple bookmarks. Note: There is already some support for it
///     in the manager classes. In all places in the code, "Shar"e and "Multiple" and "Two" are
///     keywords for file sharing between multiple bookmarks.
///     Also if user edits a shared file, he should be shown a warning and given the options to 'fork'
///     and edit the file only for the specific bookmark he is working on, or for all bookmarks.
///   - About shared files: When a bookmark is edited and we know that no other bookmark is pointing
///     to a file in filearchive, move the unused file to our own "Trash" from the file archive!
///   - Zoom in/out, real size, fit size, rotate cw/ccw buttons for image viewer, support dragging
///     and wheel zooming.
///   - Encoding and word-wrap selector for text viewer. Font selection option and wheel zooming.
///   - Changing icons of SystemApps. Especially useful for batch files and console programs.
///   - Choose which browser to open the URL with as a drop-down menu in BMViewDlg.
///   - A way to choose Global programs that appear in Open With menu of all file extensions.
///   - Show at most THREE important digits after the slash, i.e if digits before dot are less than
///     three, show more digits to the right.
///     E.g 1.00 KiB, 10.0 KiB, 110 KiB, 1.00 MiB, 10.0 MiB, 110 MiB and so on.
///   - Drag & drop attached files into Explorer to copy them!
///   - Show Shell menu for attached files, e.g with ctrl+right click or something.
///   - For installability, support creating both the database and file archive folders on a data
///     path (We have usually used `QDir::currentPath()` in such places; but it is MORE WRONG! If we
///     want the application path we need to use `QApplication::applicationDirPath()`; the current
///     working directory might be different from that. Read comments at
///     `FileManager::CreateDefaultArchives` for more info).
///   - If a file is removed manually from the file archive without program's consent, at least show
///     some warning the next time!
///   - When sorting bookmarks by URL, disregard the protocol name in sorting.
///   - Save bookmark extra data as whole JSON files and edit them using a JSON editor.
///   - Find similar urls in imported bookmarks and our collection that escaped our 'almost'
///     similar/exact check.
///   - Support full url and arg definition for Open With. This would allow e.g bat, vbs, py progs
///     to open files.
///   - Option for persisting cookies and cache, because local html previewer is easily navigatable.
///     Now maybe add a mini-browser too. Useful for occasional link clicking inside the app.
///   - MHTSave can't save e.g https://www.mozilla.org/en-US/firefox/new/. It simply doesn't show up
///     correctly!
///   - Prevent sleep while importing.
///   - Option to download MHT on bookmark creation.
///   - Option to enter a url, download and create its bookmark and then open its edit dialog!
///   - The current way of managing models and 'repopulating' them on each change is very wrong.
///     They don't need to be refreshed everytime we change the database. Eliminate them if we
///     really can't handle their complexity. However Qt is better with them, e.g drag and drop
///     support are a lot more complete for ItemViews than ItemWidgets.
///   - An 'integrity test' that checks all attached files match with database entries, deletes
///     empty folder, etc.
///   - Also, add a 'Relocate all files' which fixes the URLs of all files, because moving files and
///     folders, renaming folders, changing file archives, etc does not currently automatically fix
///     their location on file system. See the end of BookmarkFolderManager::AddOrEditBookmarkFolder.
///     In this case BookmarkFolderEdit and FileArchiveEdit dialogs must tell user that their
///     changes will not be applied on the filesystem until they select the mentioned option.
///   - More advanced search using operators, match all or any of words, etc.
///   - Filtering like "tags:whatever folder:whatever contains-this-text", and top label should show
///     what are we filtering by.

class Config
{
public:
    Config()
    {
        //// SETTINGS DEFAULT VALUES
        defaultFsTransformUnicode = "0";

        //// CONSTANTS
        concurrentBookmarkProcessings = 10;

        programDatabaseVersion = 2;
        programDatabasetFileName = "bmmgr.sqlite";

        nominalFileArchiveDirName = "FileArchive";
        fileArchiveNamePATTERN = ":arch%1:";

        nominalFileTrashDirName = "FileTrash";
        trashArchiveName = ":trash:";

        nominalFileSandBoxDirName = "FileSandBox";
        sandboxArchiveName = ":sandbox:";

        mimeTypeBookmarks = "application/x.bookmarkmanager.bookmarks";
    }
    ~Config() { }

    //// SETTINGS DEFAULT VALUES
    QString defaultFsTransformUnicode;

    //// CONSTANTS
    int concurrentBookmarkProcessings;

    int programDatabaseVersion;
    QString programDatabasetFileName;

    QString nominalFileArchiveDirName;
    QString fileArchiveNamePATTERN;

    QString nominalFileTrashDirName;
    QString trashArchiveName;

    QString nominalFileSandBoxDirName;
    QString sandboxArchiveName;

    QString mimeTypeBookmarks;
};

//UIDataDisplayRefreshAction
enum UIDDRefreshAction
{
    RA_None = 0x00,
    RA_SaveSel = 0x01,
    RA_SaveScrollPos = 0x02,
    RA_SaveSelAndScroll = RA_SaveSel | RA_SaveScrollPos,
    RA_CustomSelect = 0x04,
    RA_Focus = 0x08, //Make the selection vivid blue! Instead of gray.
    RA_SaveSelAndFocus = RA_SaveSel | RA_Focus,
    RA_SaveScrollPosAndFocus = RA_SaveScrollPos | RA_Focus,
    RA_SaveSelAndScrollAndFocus = RA_SaveSel | RA_SaveScrollPosAndFocus,
    RA_CustomSelAndSaveScrollAndFocus = RA_CustomSelect | RA_SaveScrollPosAndFocus,
    RA_CustomSelectAndFocus = RA_CustomSelect | RA_Focus,
    RA_SaveCheckState = 0x10, //Only for Tags
    RA_SaveSelAndScrollAndCheck = RA_SaveSelAndScroll | RA_SaveCheckState,
    RA_NoRefreshView = 0x20
};
