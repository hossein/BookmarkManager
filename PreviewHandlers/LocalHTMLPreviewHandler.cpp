#include "LocalHTMLPreviewHandler.h"

#include <QDebug>
#include <QFileInfo>

#include <QFrame>
#include <QHBoxLayout>
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
    QFrame* webViewFrame = new QFrame(parent);
    webViewFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

    QHBoxLayout* frameLayout = new QHBoxLayout(webViewFrame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    webViewFrame->setLayout(frameLayout);

    QWebView* webViewWidget = new QWebView(webViewFrame);
    frameLayout->addWidget(webViewWidget, 1);

    return webViewFrame;
}

bool LocalHTMLPreviewHandler::ClearAndSetDataToWidget(const QString& filePathName,
                                                      QWidget* previewWidget)
{
    QFrame* webViewFrame = qobject_cast<QFrame*>(previewWidget);
    if (webViewFrame == NULL || webViewFrame->children().size() == 0)
        return false;

    QWebView* webViewWidget = webViewFrame->findChild<QWebView*>();
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

    //The solution is finally this. This is how it works anyway; so I enclose it in Qt version checks.
    //qDebug() << filePathName << QUrl(filePathName).toString() << QUrl::fromLocalFile(filePathName).toString();

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QFileInfo fi(filePathName);
    QString lowerSuffix = fi.suffix().toLower();
    if (lowerSuffix == "mht" || lowerSuffix == "mhtml")
        webViewWidget->setUrl(QUrl::fromLocalFile(filePathName));
    else
        webViewWidget->setUrl(QUrl(filePathName));
#else
    //In Qt5 this works decently.
    webViewWidget->setUrl(QUrl::fromLocalFile(filePathName));
#endif

    return true;
}
