#pragma once
#include "IManager.h"

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
                    TransactionalFileOperator* filesTransaction);

    virtual ~IArchiveManager();

    //Pure Virtual Functions
public:
    enum ArchiveType
    {
        AT_Abstract = 0,
        AT_FileArchive,
        AT_SandBox
    };
    /// Get a unique type identifying this class (yes pure virtual functions can have implementations).
    virtual ArchiveType GetArchiveType() = 0
    {
        return AT_Abstract;
    }

    /// Note: Some derivates, e.g FAM, require that a Files Transaction MUST have been started
    /// before calling Add/Remove functions.
    virtual bool AddFileToArchive(const QString& filePathName, bool systemTrashOriginalFile,
                                  QString& fileArchiveURL) = 0;
    virtual bool RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash) = 0;

    /// We MUST always use ArchiveMan to get full file paths instead of manually appending the
    /// relative URL to the ArchiveMan root path; as some ArchiveMans might use their own strategies
    /// to store file paths.
    virtual QString GetFullArchivePathForRelativeURL(const QString& fileArchiveURL) = 0;

    //Initialization code.
public:
    /// This function create the root directory for saving the files.
    /// Derived classes can re-implement this to add extra folder structure AFTER calling base
    /// class'es `InitializeFilesDirectory()`.
    virtual bool InitializeFilesDirectory();

private:
    bool CreateLocalFileDirectory(const QString& faDirPath);

};
