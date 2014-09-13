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

    //None of these were solutions:
    //  Reading from file and setting content with custom mime type "multipart/related".
    //  Setting the URL in `webViewWidget->page()->mainFrame()->load` which is a QWebFrame.
    /*
    QFileInfo fi(filePathName);
    QString lowerSuffix = fi.suffix().toLower();
    if (lowerSuffix == "mht" || lowerSuffix == "mhtml")
    {
        //We don't do file error handling here, we're lazy!
        QFile f(filePathName);
        f.open(QIODevice::ReadOnly);
        QByteArray fileData = f.readAll();
        f.close();

        //QWebPage p;
        //QMessageBox::information(NULL, "hi", p.supportedContentTypes().join("\n"));
        webViewWidget->page()->mainFrame()->load(QUrl::fromLocalFile(filePathName));
        //webViewWidget->setContent(fileData, "multipart/x-mixed-replace");
    }
    else //HTML, etc
    {
        webViewWidget->setUrl(QUrl(filePathName));
    }
    */

    //The solution was:
    webViewWidget->setUrl(QUrl::fromLocalFile(filePathName));

    return true;
}
