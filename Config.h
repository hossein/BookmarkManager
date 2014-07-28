#pragma once
#include <QDebug>
#include <QString>

/// TODO: Plan:
///   - File Manager
///   - Make the changes that we need to require multiple file attaching and do it.
///   - Manage tag saving as comma separated integers/string or a separate table?
///   - Changes to Edit/Add UI for editing tags with mouse only
///   - Save the selection/scroll position on add/edit/delete.
///   - Preview dialog
///   - Context menu which shows a brief info and has options edit/view/show in browser, etc etc
///     (much like how Word 2010 allows changing font and formatting as well as context-menu via
///     right-click)
///   - Filter by tags
///   - "Delete" moves the bookmarks or archivefiles to a "Trash", never deletes them!
///   - Search
///   - Import Firefox bookmarks (by both browsing the json file and selecting profile!)
///
/// LATER:
///   - Make the add/edit dialog non-modal, and allow editing/viewing more than one bookmark at once
///     (but do NOT allow opening the edit/view dialog for the same bookmark twice!)
///   - Full-text index for the words inside the saved web pages!
///   - When a bookmark is edited and we know that no other bookmark is pointing to a file in
///     filearchive, move the unused file to our own "Trash!"
///   - from the file archive.
///   - Saving in zip files, and dynamic number of files per separate-directories.
///   - "Trash" viewer and manager. In Trash, tags are saved as csv texts, etc!
///   - Mass Apply Tag, AND Mass Remove Tag! Yes!
///   - Zip every 30 backups of the db and MOVE them inside a directory OTHER THAN Backups.

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
    }
    ~Config() { }

    int nominalDatabaseVersion;
    QString nominalDatabasetFileName;

    QString nominalFileArchiveDirName;
    QString fileArchivePrefix;

    QString nominalFileTrashDirName;
    QString fileTrashPrefix;
};
