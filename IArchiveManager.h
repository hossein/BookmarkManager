#pragma once
#include "IManager.h"

#include <QString>

class DatabaseManager;
class TransactionalFileOperator;

/// This interface is also known as IAM.
/// The classes that derive from this interface are known as ArchiveMans.
/// This class and DatabaseManager are the two main managers of the program, and this class needs
///   to keep a pointer to DatabaseManager, mainly for accessing application settings; because,
///   surprisingly!!, program configurations are saved in SettingsManager only and not in the
///   so-called `Config` class!
class IArchiveManager : public IManager
{
protected:
    DatabaseManager* dbm;
    /// My archive name with ':' colons at beginning and end.
    QString m_archiveName;
    QString m_archiveRoot;
    int m_fileLayout;
    TransactionalFileOperator* filesTransaction;

public:
    IArchiveManager(QWidget* dialogParent, DatabaseManager* dbm,
                    const QString& archiveName, const QString& archiveRoot,
                    int fileLayout, TransactionalFileOperator* filesTransaction);

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
    /// Also,  one must NEVER pre-calculate the file archive urls. Duplicate file names ay cause
    /// problems, especially in case of FAM.
    virtual bool AddFileToArchive(const QString& filePathName, bool systemTrashOriginalFile,
                                  const QString& folderHint, const QString& groupHint,
                                  const QString& errorWhileContext, QString& fileArchiveURL) = 0;
    virtual bool RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash,
                                       const QString& errorWhileContext) = 0;

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

/// We implement a factory class in this very file.
class ArchiveManagerFactory
{
private:
    QWidget* m_dialogParent;
    DatabaseManager* dbm;
    TransactionalFileOperator* m_filesTransaction;

public:
    ArchiveManagerFactory(QWidget* dialogParent, DatabaseManager* dbm,
                          TransactionalFileOperator* filesTransaction);

    IArchiveManager* CreateArchiveManager(IArchiveManager::ArchiveType type,
                                          const QString& archiveName, const QString& archiveRoot,
                                          int fileLayout);
};
