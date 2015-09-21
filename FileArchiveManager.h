#pragma once
#include "IArchiveManager.h"

//TODO: Save the files with their own file name in the archive!

/// This class is also known as FAM.
/// It supports two layouts:
///     Layout 0 stores files as :archivepath:/h/hash_of_filename.ext
///     Layout 1 stores files as :archivepath:/f/fi/filename.ext
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
                          QString& fileArchiveURL);
    bool RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash);

private:
    /// Could be called `CreateFileArchiveURL` too.
    /// Note: This only happens ONCE, and later if file name in archive, or any other property
    ///       that is used to calculate the hash or anyhting in the FileArchive changes, the file
    ///       remains in the folder that it always was and doesn't change location.
    ///       Also, changing the file extension does NOT change the extension that is used with
    ///       the file in the FileArchive.
    QString CalculateFileArchiveURL(const QString& fileFullPathName);
    int FileNameHash(const QString& fileNameOnly);
    QString FolderNameInitialsForASCIIChar(char c, bool startedWithPercent);

public:
    QString GetFullArchivePathForRelativeURL(const QString& fileArchiveURL);

};
