#include "IArchiveManager.h"

#include <QDir>
#include <QFileInfo>

IArchiveManager::IArchiveManager(QWidget* dialogParent, Config* conf,
                                 const QString& archiveName, const QString& archiveRoot,
                                 TransactionalFileOperator* filesTransaction)
    : IManager(dialogParent, conf), m_archiveName(archiveName), m_archiveRoot(archiveRoot)
    , filesTransaction(filesTransaction)
{
    m_archiveRoot = QDir(m_archiveRoot).absolutePath(); //Mainly to remove '/' from the end.
}

IArchiveManager::~IArchiveManager()
{

}

bool IArchiveManager::InitializeFilesDirectory()
{
    return CreateLocalFileDirectory(m_archiveRoot);
}

bool IArchiveManager::CreateLocalFileDirectory(const QString& faDirPath)
{
    //QString faDirPath = QDir::currentPath() + "/" + archiveFolderName;
    QFileInfo faDirInfo(faDirPath);

    if (!faDirInfo.exists())
    {
        if (!QDir::current().mkpath(faDirPath))
            return Error(QString("'%1' directory could not be created!").arg(faDirPath));
    }
    else if (!faDirInfo.isDir())
    {
        return Error(QString("'%1' is not a directory!").arg(faDirPath));
    }

    return true;
}
