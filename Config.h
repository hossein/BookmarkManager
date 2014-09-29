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
//  TODO: Add as note: But then we need to allow only one instance. This is also necessary do to database file usage.
//Other methods such as renaming the sandbox folder on start-up and remove it 30 seconds later, or
//  keep the list of newly opened files and don't delete them in the 30 seconds later clean up is
//  also possible; but we don't do it for now.

/// TODO: Plan:
///   x File Manager
///   x Make the changes that we need to require multiple file attaching and do it.
///   x Manage tag saving as comma separated integers/string or a separate table? (Done: the latter)
///   - Show files information on double click in BOTH add/edit and view bookmark dialogs.
///   - When user opens a file, CAN HE CHANGE IT? And should we save Changes? YES AND ALSO CHANGES
///     MUST BE SAVED IN THE DB TOO! Also, user should be warned against "changing changes multiple
///     files too". Add this last one as a note when you implemented file changing before multiple
///     file sharing.
///   x Save the selection/scroll position on add/edit/delete.
///   x View bookmark dialog
///   - In MainWindow, sort BMs by their adding date.
///   - Context menu which shows a brief info and has options edit/view/show in browser, etc etc
///     (much like how Word 2010 allows changing font and formatting as well as context-menu via
///     right-click)
///   x Filter by tags
///   - "Delete" moves the bookmarks or archivefiles to a "Trash", never deletes them!
///   - Search
///   - Import Firefox bookmarks (by both browsing the json file and selecting profile!)
///   x Extension Association Edit Dialog: For each extension, show the list of Open With (but not
///     the system default Open dialog, because we want to be system-independent and not use
///     Windows' registry CLASSES, etc) and let user (only!) Remove the apps he needs. Also a
///     dialog for the global programs that are used for every extension. Done in OpenWithDialog.
///
/// LATER:
///   - Changes to Edit/Add UI for editing tags with mouse only
///   - Make the add/edit dialog non-modal, and allow editing/viewing more than one bookmark at once
///     (but do NOT allow opening the edit/view dialog for the same bookmark twice!)
///   - Full-text index for the words inside the saved web pages!
///   - When a bookmark is edited and we know that no other bookmark is pointing to a file in
///     filearchive, move the unused file to our own "Trash" from the file archive!
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
///   - Zoom in/out, real size, fit size, rotate cw/ccw buttons for image viewer, support dragging
///     and wheel zooming.
///   - Encoding and word-wrap selector for text viewer. Font selection option and wheel zooming.
///   - Changing icons of SystemApps. Especially useful for batch files and console programs.
///   - Choose which browser to open the URL with as a drop-down menu in BMViewDlg.
///   - A way to choose Global programs that appear in Open With menu of all file extensions.
///   - Show at most THREE important digits after the slash, i.e if digits before dot are less than
///     three, show more digits to the right.
///     E.g 1.00 KiB, 10.0 KiB, 110 KiB, 1.00 MiB, 10.0 MiB, 110 MiB and so on.

class Config
{
public:
    Config()
    {
        nominalDatabaseVersion = 1;
        nominalDatabasetFileName = "bmmgr.sqlite";

        nominalFileArchiveDirName = "FileArchive";
        fileArchivePrefix = ":archive:";

        nominalFileTrashDirName = "FileTrash";
        fileTrashPrefix = ":trash:";

        nominalFileSandBoxDirName = "FileSandBox";
    }
    ~Config() { }

    int nominalDatabaseVersion;
    QString nominalDatabasetFileName;

    QString nominalFileArchiveDirName;
    QString fileArchivePrefix;

    QString nominalFileTrashDirName;
    QString fileTrashPrefix;

    QString nominalFileSandBoxDirName;
};
