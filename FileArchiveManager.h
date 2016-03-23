#pragma once
#include "IArchiveManager.h"

/// This class is also known as FAM.
/// It supports three layouts:
///     Layout 0 stores files as :archivepath:/h/hash_of_filename.ext
///     Layout 1 stores files as :archivepath:/f/fi/filename.ext
///     Layout 2 stores files as :archivepath:/folder/hint/hierarchy/[groupHint/]filename.ext
class FileArchiveManager : public IArchiveManager
{
public:
    FileArchiveManager(QWidget* dialogParent, Config* conf,
                       const QString& archiveName, const QString& archiveRoot,
                       int fileLayout, TransactionalFileOperator* filesTransaction);
    ~FileArchiveManager();

    /// Archive Type
    ArchiveType GetArchiveType()
    {
        return AT_FileArchive;
    }

    /// A Files Transaction MUST have been started before calling Add/Remove functions.
    bool AddFileToArchive(const QString& filePathName, bool systemTrashOriginalFile,
                          const QString& folderHint, const QString& groupHint,
                          const QString& errorWhileContext, QString& fileArchiveURL);
    bool RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash,
                               const QString& errorWhileContext);

private:
    /// Could be called `CreateFileArchiveURL` too. Return's a URL relative to archive root.
    /// Note: This only happens ONCE, and later if file name in archive, or any other property
    ///       that is used to calculate the hash or anyhting in the FileArchive changes, the file
    ///       remains in the folder that it always was and doesn't change location.
    ///       Also, changing the file extension does NOT change the extension that is used with
    ///       the file in the FileArchive.
    QString CalculateFileArchiveURL(const QString& fileFullPathName,
                                    const QString& folderHint, const QString& groupHint);
    int FileNameHash(const QString& fileNameOnly);
    ///FolderHierForName returns returns 'f/fi/' for 'fileName'.
    QString FolderHierForName(const QString& name, bool isFileName);
    QString FolderNameInitialsForChar(int c);

public:
    ///Utility function
    ///`isFileName=true` will not try to find illegal characters for file names, however it
    /// recognizes and re-adds a file extension (only if the extension is less 20 characters).
    /// The output will contain only ASCII characters.
    static QString SafeAndShortFSName(const QString& fsName, bool isFileName);

public:
    QString GetFullArchivePathForRelativeURL(const QString& fileArchiveURL);

};
