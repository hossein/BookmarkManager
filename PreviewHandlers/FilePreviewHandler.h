#pragma once
#include <QWidget>
#include <QString>
#include <QStringList>

class FileViewManager;

/// Preview handler for files. This class associates itself with some extensions and is called
/// by FileViewManager to fill in a QWidget* preview class that it has created.
///
/// FilePreviewHandlers MUST NOT BE ABLE TO edit the file contents, as original files from the
/// FileArchive/Trash are passed to them.

class FilePreviewHandler
{
public:
    FilePreviewHandler() { }
    virtual ~FilePreviewHandler() { }

    /// This function must instantiates all subclasses of this interface and add them to the
    /// given FileViewManager.
    static void InstantiateAllKnownFilePreviewHandlersInFileViewManager(FileViewManager* fview);

    /// Must return a name unique to all FilePreviewHandler's.
    /// This is used to tie this handler to the widget it creates.
    virtual QString GetUniqueName() = 0;

    /// Must return the LOWER-CASE list of extensions this supports.
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
    /// Must clear the widget before showing the new data for the sake of itself, unless encounters
    /// error. If successful, must return true; otherwise must return false and doesn't have to
    /// clear the widget too.
    virtual bool ClearAndSetDataToWidget(const QString& filePathName, QWidget* previewWidget) = 0;
};
