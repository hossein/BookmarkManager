#pragma once
#include "IArchiveManager.h"

class FileSandBoxManager : public IArchiveManager
{
public:
    FileSandBoxManager(QWidget* dialogParent, Config* conf,
                       const QString& archiveName, const QString& archiveRoot,
                       TransactionalFileOperator* filesTransaction);
    ~FileSandBoxManager();

    /// Extra function of this class.
    bool ClearSandBox();

    /// Doesn't require Files Transaction.
    bool AddFileToArchive(const QString& filePathName, bool systemTrashOriginalFile,
                          QString& fileArchiveURL);
    bool RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash);

    QString GetFullArchivePathForRelativeURL(const QString& fileArchiveURL);

};
