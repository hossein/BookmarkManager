#pragma once
#include "ISubManager.h"
#include <QString>
#include <QStringList>

class DatabaseManager;

class FileViewManager : public ISubManager
{
    friend class DatabaseManager;

public:
    FileViewManager(QWidget* dialogParent, Config* conf);

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
