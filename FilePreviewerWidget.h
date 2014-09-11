#pragma once
#include <QWidget>

#include <QHash>
#include <QString>

class QLabel;
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
    QLabel* m_emptyFilePreviewLbl;
    QHash<QString,QWidget*> m_widgetsCreatedByPreviewHandlers;

public:
    explicit FilePreviewerWidget(QWidget *parent = 0);

    /// Clears any previews of the previous files.
    void ClearPreview();

    /// filePathName must be absolute, as it is passed to the FilePreviewHandler.
    void PreviewFileUsingPreviewHandler(const QString& filePathName, FilePreviewHandler* fph);


signals:

public slots:

};
