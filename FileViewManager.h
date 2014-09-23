#pragma once
#include "ISubManager.h"
#include <QHash>
#include <QString>
#include <QStringList>
#include <QPixmap>

class QMenu;
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

    // Preview Handlers ///////////////////////////////////////////////////////////////////////////
public:
    /// Determines the extensions automatically.
    /// This class will OWN the preview handlers added here and removes them in destructor.
    void AddPreviewHandler(FilePreviewHandler* fph);

    /// Only file extension will be verified.
    bool HasPreviewHandler(const QString& fileName);

private:
    /// Only file extension will be verified.
    /// Returns NULL if there isn't a preview handler for the extension.
    FilePreviewHandler* GetPreviewHandler(const QString& fileName);

    // Other Non-DB Functions /////////////////////////////////////////////////////////////////////
public:
    /// Only file extensions will be verified.
    int ChooseADefaultFileBasedOnExtension(const QStringList& filesList);

    /// Makes a nice 'Open With >' menu.
    /// Only file extension will be verified.
    void PopulateOpenWithMenu(const QString& fileName, QMenu* parentMenu,
                              const QObject* receiver, const char* member);
    enum OpenWithSpecialValues
    {
        OWS_OpenWithSystemDefault = -1,
        OWS_OpenWithDialogRequest = -2
    };

    // File Opening Functions /////////////////////////////////////////////////////////////////////
public:
    /// In ALL of the following functions:
    /// `filePathName` MUST BE ABSOLUTE file path! NOT :archive: path.
    /// (as this class is an ISubManager itself and doesn't have access to FileManager to resolve
    ///  the :archive: URL into an absolute path).
    void Preview(const QString& filePathName, FilePreviewerWidget* fpw);
    void OpenReadOnly(const QString& filePathName, FileManager* files);
    void OpenEditable(const QString& filePathName, FileManager* files);
    void OpenWith(const QString& filePathName, DatabaseManager* dbm, QWidget* dialogParent);
    void ShowProperties(const QString& filePathName);

    ///If !sandboxed, then files can be NULL.
    void GenericOpenFile(const QString& filePathName, long long programSAID,
                         bool sandboxed, FileManager* files);
private:
    void DirectOpenFile(const QString& filePathName, long long programSAID);

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

private:
    //Don't use directly; for the same reason as preferredOpenProgram.
    QHash<QString, QList<long long>> associatedOpenPrograms;

    struct ExtOpenWithData
    {
        ExtOpenWithData() { }
        ExtOpenWithData(long long EOWID, long long SAID) : EOWID(EOWID), SAID(SAID) { }
        long long EOWID;
        long long SAID;
    };

    //Don't use directly; many extensions don't have a record in this QHash.
    //  Use `GetPreferredOpenApplication` to return `-1` for those.
    QHash<QString, ExtOpenWithData> preferredOpenProgram;

public:
    /// This function is only called once upon startup. Other functions are [RESPONSIBLE] for
    /// keeping the internal tables updated afterwards.
    void PopulateInternalTables();

    //SystemApp
    /// SAID can only be -1 for adding, not anything else.
    bool AddOrEditSystemApp(long long& SAID, SystemAppData& sadata);

    //ExtAssoc
    /// Only file extension will be verified. Returns an empty list if there isn't any association
    /// for the default program. TODO: What about the default '' extension?
    QList<long long> GetAssociatedOpenApplications(const QString& fileName);

    /// If the file type is already associated, this function returns `true` without modifying
    /// the database. Setting to -1 also returns true without modifying the db.
    bool AssociateApplicationWithExtension(const QString& fileName, long long associatedSAID);

    /// Does nothing if the application is not already associated with the extension.
    bool UnAssociateApplicationWithExtension(const QString& fileName, long long associatedSAID);

    //ExtOpenWith
    /// Only file extension will be verified.
    /// Returns -1 if either there isn't a preferred application or the user has explicitly
    /// preferred to open with default system application. Anyway in -1 case we should open with
    /// the default system app.
    long long GetPreferredOpenApplication(const QString& fileName);

    /// Setting to -1 doesn't remove the database entry or anything (as if it should, to save space).
    bool SetPreferredOpenApplication(const QString& fileName, long long preferredSAID);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
