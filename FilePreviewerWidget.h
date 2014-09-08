#pragma once
#include <QWidget>

#include <QHash>
#include <QString>

class QStackedLayout;
class FilePreviewHandler;

/// This class manages a list of FilePreviewHandler's and their corresponding widgets to prevent
/// creating a new widget for the same FilePreviewHandler twice. It can be directly used to show
/// the preview of the files.
class FilePreviewerWidget : public QWidget
{
    Q_OBJECT

private:
    QStackedLayout* m_layout;
    QWidget* m_emptyFilePreview;
    QHash<QString,QWidget*> m_widgetsCreatedByPreviewHandlers;

public:
    explicit FilePreviewerWidget(QWidget *parent = 0);

    /// filePathName must be absolute, as it is passed to the FilePreviewHandler.
    void PreviewFileUsingPreviewHandler(const QString& filePathName, FilePreviewHandler* fph);


signals:

public slots:

};
