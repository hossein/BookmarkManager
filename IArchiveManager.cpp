#include "IArchiveManager.h"

#include <QDir>
#include <QFileInfo>

//Include our own archive managers for the ArchiveManagerFactory.
#include "FileArchiveManager.h"
#include "FileSandBoxManager.h"

IArchiveManager::IArchiveManager(QWidget* dialogParent, Config* conf,
                                 const QString& archiveName, const QString& archiveRoot,
                                 int fileLayout, TransactionalFileOperator* filesTransaction)
    : IManager(dialogParent, conf), m_archiveName(archiveName), m_archiveRoot(archiveRoot)
    , m_fileLayout(fileLayout), filesTransaction(filesTransaction)
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
    //Not specifying the QDir::currentPath() is the same as specifying it wrt relative path names!
    //  However by not specifying we actually let relative creation, too; although other parts
    //  call this function only with absolute paths.
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



ArchiveManagerFactory::ArchiveManagerFactory(QWidget* dialogParent, Config* conf,
                                             TransactionalFileOperator* filesTransaction)
    : m_dialogParent(dialogParent), m_conf(conf), m_filesTransaction(filesTransaction)
{

}

IArchiveManager* ArchiveManagerFactory::CreateArchiveManager(IArchiveManager::ArchiveType type,
                                                             const QString& archiveName,
                                                             const QString& archiveRoot,
                                                             int fileLayout)
{
    IArchiveManager* iam = NULL;
    switch (type)
    {
    case IArchiveManager::AT_FileArchive:
        iam = new FileArchiveManager(m_dialogParent,m_conf,archiveName,archiveRoot,fileLayout,m_filesTransaction);
        break;
    case IArchiveManager::AT_SandBox:
        iam = new FileSandBoxManager(m_dialogParent,m_conf,archiveName,archiveRoot,m_filesTransaction);
        break;
    }
    return iam;
}
