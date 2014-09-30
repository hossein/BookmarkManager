#pragma once
#include "IArchiveManager.h"

/// This class is also known as FAM.
class FileArchiveManager : public IArchiveManager
{
public:
    FileArchiveManager(QWidget* dialogParent, Config* conf,
                       const QString& archiveName, const QString& archiveRoot,
                       TransactionalFileOperator* filesTransaction);
    ~FileArchiveManager();

    /// Archive Type
    virtual ArchiveType GetArchiveType()
    {
        return AT_FileArchive;
    }

    /// A Files Transaction MUST have been started before calling Add/Remove functions. And TODO: Check this.
    bool AddFileToArchive(const QString& filePathName, bool systemTrashOriginalFile,
                          QString& fileArchiveURL);
    bool RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash);

private:
    /// Could be called `CreateFileArchiveURL` too.
    /// Note: This only happens ONCE, and later if file name in archive, or any other property
    ///       that is used to calculate the hash or anyhting in the FileArchive changes, the file
    ///       remains in the folder that it always was and doesn't change location.
    ///       Also, remaining the file extension does NOT change the extension that is used with
    ///       the file in the FileArchive.
    QString CalculateFileArchiveURL(const QString& fileFullPathName);
    int FileNameHash(const QString& fileNameOnly);

public:
    QString GetFullArchivePathForRelativeURL(const QString& fileArchiveURL);

};
