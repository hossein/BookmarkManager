#pragma once
#include "ISubManager.h"
#include <QHash>
#include <QString>
#include <QStringList>

class DatabaseManager;
class FileManager;
class FilePreviewHandler;
class FilePreviewerWidget;

class FileViewManager : public ISubManager
{
    friend class DatabaseManager;

    QList<FilePreviewHandler*> m_ownedPreviewHandlers;
    QHash<QString,FilePreviewHandler*> m_extensionsPreviewHandlers;

public:
    FileViewManager(QWidget* dialogParent, Config* conf);
    ~FileViewManager();

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

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
