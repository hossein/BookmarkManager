#pragma once
#include <QList>
#include <QString>
#include <QStringList>

/// Provide transactional file management. Only one transaction can be active at a time.
class TransactionalFileOperator
{
private:
    enum FileAction
    {
        FA_Rename,
        FA_Copy,
        FA_SystemTrash,
        FA_Delete
    };

    bool fileTransactionStarted;

    QList<FileAction> actions;
    QStringList srcFiles;
    QStringList destFiles;

public:
    TransactionalFileOperator();

    bool BeginTransaction();
    bool CommitTransaction();
    bool RollBackTransaction();

public:
    /// Rename can be used for BOTH renaming and MOVING AS LONG AS the files are on the same volume.
    bool RenameFile(const QString& oldName, const QString& newName);
    bool CopyFile(const QString& oldPath, const QString& newPath);
    bool SystemTrashFile(const QString& filePath);
    bool DeleteFile(const QString& filePath);

private:
    void EndTransaction();
};
