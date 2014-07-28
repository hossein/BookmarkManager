#include "TransactionalFileOperator.h"

#include "WinFunctions.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

TransactionalFileOperator::TransactionalFileOperator()
{
    fileTransactionStarted = false;
}

bool TransactionalFileOperator::BeginTransaction()
{
    if (fileTransactionStarted)
        return false;

    fileTransactionStarted = true;
    return true;
}

bool TransactionalFileOperator::CommitTransaction()
{
    EndTransaction();
    return true;
}

bool TransactionalFileOperator::RollBackTransaction()
{
//TODO: ROLL BACK
    EndTransaction();
    return true;
}

bool TransactionalFileOperator::RenameFile(const QString& oldName, const QString& newName)
{
    bool result = QFile::rename(oldName, newName);

    if (result)
    {
        actions.append(FA_Rename);
        srcFiles.append(oldName);
        destFiles.append(newName);
    }

    return result;
}

bool TransactionalFileOperator::CopyFile(const QString& oldPath, const QString& newPath)
{
    bool result = QFile::copy(oldPath, newPath);

    if (result)
    {
        actions.append(FA_Copy);
        srcFiles.append(oldPath);
        destFiles.append(newPath);
    }

    return result;
}

bool TransactionalFileOperator::SystemTrashFile(const QString& filePath)
{
    bool result = WinFunctions::MoveFileToRecycleBin(filePath);

    if (result)
    {
        actions.append(FA_SystemTrash);
        srcFiles.append(filePath);
        destFiles.append(""); //Empty item important.
    }

    return result;
}

bool TransactionalFileOperator::DeleteFile(const QString& filePath)
{
    bool result = QFile::remove(filePath);

    if (result)
    {
        actions.append(FA_Delete);
        srcFiles.append(filePath);
        destFiles.append(""); //Empty item important.
    }

    return result;
}

void TransactionalFileOperator::EndTransaction()
{
    fileTransactionStarted = false;
    actions.clear();
    srcFiles.clear();
    destFiles.clear();
}
