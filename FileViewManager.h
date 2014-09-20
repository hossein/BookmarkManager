#pragma once
#include "ISubManager.h"
#include <QHash>
#include <QString>
#include <QStringList>
#include <QPixmap>

class DatabaseManager;
class FileManager;
class FilePreviewHandler;
class FilePreviewerWidget;

class FileViewManager : public ISubManager
{
    friend class DatabaseManager;

private:
    QList<FilePreviewHandler*> m_ownedPreviewHandlers;
    QHash<QString,FilePreviewHandler*> m_extensionsPreviewHandlers;

public:
    FileViewManager(QWidget* dialogParent, Config* conf);
    ~FileViewManager();

    // Non-DB Functions ///////////////////////////////////////////////////////////////////////////
public:
    /// Determines the extensions automatically.
    /// This class will OWN the preview handlers added here and removes them in destructor.
    void AddPreviewHandler(FilePreviewHandler* fph);

    /// Only file extensions will be verified.
    int ChooseADefaultFileBasedOnExtension(const QStringList& filesList);

    /// Only file extension will be verified.
    bool HasPreviewHandler(const QString& fileName);

    /// `filePathName` MUST BE ABSOLUTE file path! NOT :archive: path.
    /// (as this class is an ISubManager itself and doesn't have access to FileManager to resolve
    ///  the :archive: URL into an absolute path).
    void Preview(const QString& filePathName, FilePreviewerWidget* fpw);
    void OpenReadOnly(const QString& filePathName, FileManager* files);
    void OpenEditable(const QString& filePathName, FileManager* files);
    void OpenWith(const QString& filePathName, DatabaseManager* dbm);
    void ShowProperties(const QString& filePathName);

private:
    /// Only file extension will be verified.
    /// Returns NULL if there isn't a preview handler for the extension.
    FilePreviewHandler* GetPreviewHandler(const QString& fileName);

    // Database Functions /////////////////////////////////////////////////////////////////////////
public:
    struct SystemAppData
    {
        long long SAID;
        QString Name;
        QString Path;
        QPixmap SmallIcon;
        QPixmap LargeIcon;
    };

    QHash<long long, SystemAppData> systemApps;

    void PopulateSystemAppsList();
    bool AddOrEditSystemApp(long long& SAID, SystemAppData& sadata);

    /// Only file extension will be verified.
    /// Sets `preferredSAID` to -1 if either there isn't a preferred application or the user has
    /// explicitly preferred to open with default system application. In both cases we should open
    /// with the default system app.
    bool GetPreferredOpenApplication(const QString& fileName, long long& preferredSAID);

    /// Setting to -1 doesn't remove the database entry or anything (as if it should, to save space).
    bool SetPreferredOpenApplication(const QString& fileName, long long preferredSAID);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
