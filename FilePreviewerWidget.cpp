#include "FilePreviewerWidget.h"

#include "PreviewHandlers/FilePreviewHandler.h"

#include <QStackedLayout>

FilePreviewerWidget::FilePreviewerWidget(QWidget *parent) :
    QWidget(parent)
{
    m_layout = new QStackedLayout();
    this->setLayout(m_layout);

    m_emptyFilePreview = new QWidget(this);
    m_layout->addWidget(m_emptyFilePreview);
}

void FilePreviewerWidget::PreviewFileUsingPreviewHandler(const QString& filePathName,
                                                         FilePreviewHandler* fph)
{
    QWidget* previewWidget;

    QString fphUniqueName = fph->GetUniqueName();
    if (m_widgetsCreatedByPreviewHandlers.contains(fphUniqueName))
    {
        previewWidget = m_widgetsCreatedByPreviewHandlers[fphUniqueName];
    }
    else
    {
        previewWidget = fph->CreateAndFreeWidget(this);
        m_layout->addWidget(previewWidget);
    }

    m_layout->setCurrentWidget(previewWidget);

    bool success = fph->ClearAndSetDataToWidget(filePathName, previewWidget);

    //In case of failing, show an empty preview.
    if (!success)
        m_layout->setCurrentWidget(m_emptyFilePreview);
}
