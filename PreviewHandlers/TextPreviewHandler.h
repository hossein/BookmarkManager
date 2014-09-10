#pragma once
#include "FilePreviewHandler.h"

class TextPreviewHandler : public FilePreviewHandler
{
public:
    TextPreviewHandler();

    // FilePreviewHandler interface
public:
    QString GetUniqueName();
    QStringList GetSupportedExtensions();
    FileCategory GetFilesCategory();

    QWidget* CreateAndFreeWidget(QWidget* parent);
    bool ClearAndSetDataToWidget(const QString& filePathName, QWidget* previewWidget);
};
