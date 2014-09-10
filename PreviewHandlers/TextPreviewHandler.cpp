#include "TextPreviewHandler.h"
#include <QPlainTextEdit>

TextPreviewHandler::TextPreviewHandler()
{
}

QString TextPreviewHandler::GetUniqueName()
{
    return "Def.BookmarkManager.PreviewHandler.Text";
}

QStringList TextPreviewHandler::GetSupportedExtensions()
{
    return QStringList() << "txt" << "log";
}

FilePreviewHandler::FileCategory TextPreviewHandler::GetFilesCategory()
{
    return FC_Text;
}

QWidget* TextPreviewHandler::CreateAndFreeWidget(QWidget* parent)
{
    QPlainTextEdit* textWidget = new QPlainTextEdit(parent);
    textWidget->setReadOnly(true);
    return textWidget;
}

bool TextPreviewHandler::ClearAndSetDataToWidget(const QString& filePathName, QWidget* previewWidget)
{
    QPlainTextEdit* textWidget = qobject_cast<QPlainTextEdit*>(previewWidget);
    if (textWidget == NULL)
        return false;

    QFile textFile(filePathName);
    textFile.open(QFile::ReadOnly);
    QByteArray textFileBytes = textFile.readAll();
    if (textFile.error() != QFile::NoError)
        return false;

    textFile.close();
    QString textFileContents = QString::fromUtf8(textFileBytes.data(), textFileBytes.size());

    textWidget->setPlainText(textFileContents);
    return true;
}
