#include "LocalHTMLPreviewHandler.h"

#include <QFileInfo>
#include <QApplication>

#include <QBoxLayout>
#include <QFrame>
#include <QProgressBar>
#include <QToolButton>
#include <QWebView>

void LocalHTMLPreviewCursorChanger::SetBusyLoadingCursor()
{
    qApp->setOverrideCursor(Qt::BusyCursor);
}

void LocalHTMLPreviewCursorChanger::RestoreCursor()
{
    qApp->restoreOverrideCursor();
}

LocalHTMLLoadButtonEnabler::LocalHTMLLoadButtonEnabler(
        QToolButton* refreshButton, QToolButton* stopButton, QWebView* webView, QObject* parent)
    : QObject(parent), m_refreshButton(refreshButton), m_stopButton(stopButton), m_webView(webView)
{
    connect(m_webView, SIGNAL(loadStarted()), this, SLOT(WebViewLoadStarted()));
    connect(m_webView, SIGNAL(loadFinished(bool)), this, SLOT(WebViewLoadFinished()));
}

void LocalHTMLLoadButtonEnabler::WebViewLoadStarted()
{
    m_refreshButton->setEnabled(false);
    m_stopButton->setEnabled(true);
}

void LocalHTMLLoadButtonEnabler::WebViewLoadFinished()
{
    m_refreshButton->setEnabled(true);
    m_stopButton->setEnabled(false);
}

LocalHTMLPreviewHandler::LocalHTMLPreviewHandler()
{
    m_cursorChanger = new LocalHTMLPreviewCursorChanger;
}

LocalHTMLPreviewHandler::~LocalHTMLPreviewHandler()
{
    m_cursorChanger->deleteLater();
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

    QVBoxLayout* frameLayout = new QVBoxLayout(webViewFrame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(0);
    webViewFrame->setLayout(frameLayout);

    QWidget* toolbarWidget = new QWidget(webViewFrame);
    frameLayout->addWidget(toolbarWidget);

    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarWidget->setLayout(toolbarLayout);

    QToolButton* tbRefresh = new QToolButton(toolbarWidget);
    tbRefresh->setIcon(QIcon(":/res/web_refresh.png"));
    tbRefresh->setText("Refresh");
    tbRefresh->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbarLayout->addWidget(tbRefresh);

    QToolButton* tbStop = new QToolButton(toolbarWidget);
    tbStop->setIcon(QIcon(":/res/web_stop.png"));
    tbStop->setText("Stop");
    tbStop->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbarLayout->addWidget(tbStop);

    toolbarLayout->addStretch(1);

    QProgressBar* prgLoadProgress = new QProgressBar(toolbarWidget);
    prgLoadProgress->setFixedWidth(100);
    prgLoadProgress->setMinimum(0);
    prgLoadProgress->setMaximum(100);
    prgLoadProgress->setValue(0);
    prgLoadProgress->setTextVisible(false);
    toolbarLayout->addWidget(prgLoadProgress);

    QWebView* webViewWidget = new QWebView(webViewFrame);
    frameLayout->addWidget(webViewWidget, 1);

    //Loading and rendering webkit contents os a long asynchronous operation, so we manage cursors.
    //Loading of a page may take forever, so we also added a toolbar, which turned to be a nice thing.
    webViewWidget->connect(webViewWidget, SIGNAL(loadStarted()), m_cursorChanger, SLOT(SetBusyLoadingCursor()));
    webViewWidget->connect(webViewWidget, SIGNAL(loadFinished(bool)), m_cursorChanger, SLOT(RestoreCursor()));

    //Start, stop, progress
    webViewWidget->connect(tbRefresh, SIGNAL(clicked()), webViewWidget, SLOT(reload()));
    webViewWidget->connect(tbStop, SIGNAL(clicked()), webViewWidget, SLOT(stop()));
    webViewWidget->connect(webViewWidget, SIGNAL(loadProgress(int)), prgLoadProgress, SLOT(setValue(int)));

    //Can't connect web view's load started/finished signal to toolbuttons' setenabled/disabled
    //  because the slots need required arguments. So we use a helper class.
    //Unlike m_cursorChanger, this keeps references to items so it should get deleted together with
    //  the web view widget, hence assignment of webViewWidget as parent. Instantiating it is enough.
    //  (Actually now I think more about it we should be able to just define one of it, anyway if the
    //  objects are destroyed their signal/slots connection is destroyed automatically too.)
    new LocalHTMLLoadButtonEnabler(tbRefresh, tbStop, webViewWidget, webViewWidget);

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
