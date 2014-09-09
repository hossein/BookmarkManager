#pragma once
#include "FilePreviewHandler.h"

class ImagePreviewHandler : public FilePreviewHandler
{
public:
    ImagePreviewHandler();

    // FilePreviewHandler interface
public:
    QString GetUniqueName();
    QStringList GetSupportedExtensions();
    FileCategory GetFilesCategory();

    QWidget* CreateAndFreeWidget(QWidget* parent);
    bool ClearAndSetDataToWidget(const QString& filePathName, QWidget* previewWidget);
};
