#include "LocalHTMLPreviewHandler.h"
#include <QWebView>

LocalHTMLPreviewHandler::LocalHTMLPreviewHandler()
{
}

QString LocalHTMLPreviewHandler::GetUniqueName()
{
    return "Def.BookmarkManager.PreviewHandler.LocalHTML";
}

QStringList LocalHTMLPreviewHandler::GetSupportedExtensions()
{
    return QStringList() << "htm" << "html" << "mht" << "mhtml";
}

FilePreviewHandler::FileCategory LocalHTMLPreviewHandler::GetFilesCategory()
{
    return FC_LocalHTML;
}

QWidget* LocalHTMLPreviewHandler::CreateAndFreeWidget(QWidget* parent)
{
    QWebView* webViewWidget = new QWebView(parent);
    return webViewWidget;
}

bool LocalHTMLPreviewHandler::ClearAndSetDataToWidget(const QString& filePathName,
                                                      QWidget* previewWidget)
{
    QWebView* webViewWidget = qobject_cast<QWebView*>(previewWidget);
    if (webViewWidget == NULL)
        return false;

    webViewWidget->setUrl(QUrl(filePathName));
    return true;
}
