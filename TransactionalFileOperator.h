#pragma once
#include <QList>
#include <QString>

/// Provide transactional file management. Only one transaction can be active at a time.
class TransactionalFileOperator
{
private:
    struct FileOp
    {
        enum FileActionType
        {
            FAT_MakePath,
            FAT_Rename,
            FAT_Copy,
            FAT_Move,
            FAT_SystemTrash,
            FAT_Delete
        };

        FileOp() { }
        FileOp(FileActionType action, const QString& srcFile, const QString& destFile)
            : action(action), srcFile(srcFile), destFile(destFile)
        { }

        FileActionType action;
        QString srcFile;
        QString destFile;
    };

    bool fileTransactionStarted;
    QList<FileOp> fileOps;

public:
    TransactionalFileOperator();

    bool BeginTransaction();
    bool CommitTransaction();
    bool RollBackTransaction();

public:
    bool MakePath(const QString& basePath, const QString& pathToMake);
    /// Rename can be used for BOTH renaming and MOVING AS LONG AS the files are on the same volume.
    bool RenameFile(const QString& oldName, const QString& newName);
    bool CopyFile(const QString& oldPath, const QString& newPath);
    bool MoveFile(const QString& oldPath, const QString& newPath);
    bool SystemTrashFile(const QString& filePath);
    bool DeleteFile(const QString& filePath);

private:
    void EndTransaction();
    bool backupFileInTemp(const QString& filePath, QString& backUpFilePath);
};
