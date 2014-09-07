#pragma once
#include "ISubManager.h"
#include <QHash>
#include <QString>
#include <QStringList>

class DatabaseManager;
class FilePreviewHandler;

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

    /// `filePathName` in the following group can be BOTH :archive: path and a real path.
    void Preview(const QString& filePathName);
    void OpenReadOnly(const QString& filePathName);
    void OpenEditable(const QString& filePathName);
    void OpenWith(const QString& filePathName);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
