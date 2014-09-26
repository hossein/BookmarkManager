#pragma once
#include "IManager.h"

#include <QDir>
#include <QString>

class TransactionalFileOperator;

/// This interface is also known as IAM.
/// The classes that derive from this interface are known as ArchiveMans.
class IArchiveManager : public IManager
{
protected:
    /// My archive name with ':' colons at beginning and end.
    QString m_archiveName;
    QString m_archiveRoot;
    TransactionalFileOperator* filesTransaction;

public:
    IArchiveManager(QWidget* dialogParent, Config* conf,
                    const QString& archiveName, const QString& archiveRoot,
                    TransactionalFileOperator* filesTransaction)
        : IManager(dialogParent, conf), m_archiveName(archiveName), m_archiveRoot(archiveRoot)
        , filesTransaction(filesTransaction)
    {
        //TODO: If archive folder doesn't exist, must create it here, not anywhere else.
        //Add as a note that children can add extra folder structure in their constructors.

        m_archiveRoot = QDir(m_archiveRoot).absolutePath(); //Mainly to remove '/' from the end.
    }

    virtual ~IArchiveManager()
    {

    }

    /// Note: Some derivates, e.g FAM, require that a Files Transaction MUST have been started
    /// before calling Add/Remove functions.
    virtual bool AddFileToArchive(const QString& filePathName, bool removeOriginalFile,
                                  QString& fileArchiveURL) = 0;
    virtual bool RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash) = 0;

    /// We MUST always use ArchiveMan to get full file paths instead of manually appending the
    /// relative URL to the ArchiveMan root path; as some ArchiveMans might use their own strategies
    /// to store file paths.
    virtual QString GetFullArchivePathForRelativeURL(const QString& fileArchiveURL) = 0;

};
