#pragma once
#include <QWidget>
#include <QString>
#include <QStringList>

/// Preview handler for files. This class associates itself with some extensions and is called
/// by FileViewManager to fill in a QWidget* preview class that it has created.
class FilePreviewHandler
{
public:
    FilePreviewHandler() { }
    virtual ~FilePreviewHandler() { }

    virtual QString GetUniqueName() = 0;
    virtual QStringList GetSupportedExtensions() = 0;

    enum FileCategory
    {
        FC_LocalHTML,
        FC_Web,
        FC_Text,
        FC_Code,
        FC_Document,
        FC_Archive,
        FC_Image,
        FC_Music,
        FC_Video,
        FC_Software,
    };
    virtual FileCategory GetFilesCategory() = 0;

    /// The created widget must not be bound to this FilePreviewHandler; even if it is,
    /// it must be able to be deleted by the calling class/container.
    virtual QWidget* CreateAndFreeWidget(QWidget* parent) = 0;

    /// filePathName must be ABSOLUTE file path on file system and NOT :archive:.
    /// FileViewManager that uses this class knows this fact.
    virtual bool ClearAndSetDataToWidget(const QString& filePathName, QWidget* previewWidget) = 0;
};
