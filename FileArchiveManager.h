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

    /// A Files Transaction MUST have been started before calling this function. And TODO: Check this.
    bool AddFileToArchive(const QString& filePathName, bool removeOriginalFile,
                          QString& fileArchiveURL);

private:
    /// Could be called `CreateFileArchiveURL` too.
    /// Note: This only happens ONCE, and later if file name in archive, or any other property
    ///       that is used to calculate the hash or anyhting in the FileArchive changes, the file
    ///       remains in the folder that it always was and doesn't change location.
    ///       Also, remaining the file extension does NOT change the extension that is used with
    ///       the file in the FileArchive.
    QString CalculateFileArchiveURL(const QString& fileFullPathName);
    QString GetFullArchivePathForFile(const QString& fileArchiveURL);
    int FileNameHash(const QString& fileNameOnly);
};
