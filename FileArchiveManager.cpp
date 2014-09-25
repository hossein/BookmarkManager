#include "FileArchiveManager.h"

#include "TransactionalFileOperator.h"

FileArchiveManager::FileArchiveManager(QWidget* dialogParent, Config* conf,
                                       const QString& archiveName, const QString& archiveRoot,
                                       TransactionalFileOperator* filesTransaction)
    : IManager(dialogParent, conf), m_archiveName(archiveName), m_archiveRoot(archiveRoot)
    , filesTransaction(filesTransaction)
{
}

FileArchiveManager::~FileArchiveManager()
{

}
