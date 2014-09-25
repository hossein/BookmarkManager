#pragma once
#include "IManager.h"

#include <QString>

class TransactionalFileOperator;

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
};
