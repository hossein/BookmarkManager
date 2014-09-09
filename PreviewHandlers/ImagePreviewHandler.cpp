#include "ImagePreviewHandler.h"

#include <QLabel>
#include <QScrollArea>

ImagePreviewHandler::ImagePreviewHandler()
{
}

QString ImagePreviewHandler::GetUniqueName()
{
    return "Def.BookmarkManager.PreviewHandler.Image";
}

QStringList ImagePreviewHandler::GetSupportedExtensions()
{
    return QStringList() << "bmp" << "gif" << "jpg" << "jpeg" << "png" << "pbm" << "pgm"
                         << "ppm" << "xbm" << "xpm";
}

FilePreviewHandler::FileCategory ImagePreviewHandler::GetFilesCategory()
{
    return FC_Image;
}

QWidget* ImagePreviewHandler::CreateAndFreeWidget(QWidget* parent)
{
    QScrollArea* imageScrollWidget = new QScrollArea(parent);
    imageScrollWidget->setBackgroundRole(QPalette::Dark);
    imageScrollWidget->setContentsMargins(0, 0, 0, 0);

    QLabel* imageLabel = new QLabel(imageScrollWidget);
    imageLabel->setContentsMargins(0, 0, 0, 0);

    imageScrollWidget->setWidget(imageLabel);
    return imageScrollWidget;
}

bool ImagePreviewHandler::ClearAndSetDataToWidget(const QString& filePathName, QWidget* previewWidget)
{
    QScrollArea* imageScrollWidget = qobject_cast<QScrollArea*>(previewWidget);
    if (imageScrollWidget == NULL)
        return false;

    QLabel* imageLabel = qobject_cast<QLabel*>(imageScrollWidget->widget());
    if (imageLabel == NULL)
        return false;

    QPixmap filePixmap(filePathName);
    if (filePixmap.isNull()) //In case of errors
        return false;

    imageLabel->setPixmap(filePixmap);
    imageLabel->resize(filePixmap.size());
    return true;
}
