#pragma
#include "FilePreviewHandler.h"

class LocalHTMLPreviewHandler : public FilePreviewHandler
{
public:
    LocalHTMLPreviewHandler();

    // FilePreviewHandler interface
public:
    QString GetUniqueName();
    QStringList GetSupportedExtensions();
    FileCategory GetFilesCategory();

    QWidget* CreateAndFreeWidget(QWidget* parent);
    bool ClearAndSetDataToWidget(const QString& filePathName, QWidget* previewWidget);
};
