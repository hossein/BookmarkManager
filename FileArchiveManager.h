#pragma once
#include "IManager.h"

#include <QString>

class TransactionalFileOperator;

/// This class is also known as FAM.
class FileArchiveManager : public IManager
{
private:
    /// My archive name with ':' colons at beginning and end.
    QString m_archiveName;
    QString m_archiveRoot;
    TransactionalFileOperator* filesTransaction;

public:
    FileArchiveManager(QWidget* dialogParent, Config* conf,
                       const QString& archiveName, const QString& archiveRoot,
                       TransactionalFileOperator* filesTransaction);
    ~FileArchiveManager();

    /// A Files Transaction MUST have been started before calling Add/Remove functions. And TODO: Check this.
    bool AddFileToArchive(const QString& filePathName, bool removeOriginalFile,
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
    /// We MUST always use FAM to get full file paths instead of manually appending the relative URL
    /// to the FAM root path; as some FAMs might use their own strategies to store file paths.
    QString GetFullArchivePathForRelativeURL(const QString& fileArchiveURL);

};
