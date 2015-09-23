#pragma
#include <QObject>
#include "FilePreviewHandler.h"

class LocalHTMLPreviewCursorChanger : public QObject
{
    Q_OBJECT
public:
    LocalHTMLPreviewCursorChanger(QObject* parent = NULL): QObject(parent) { }
    ~LocalHTMLPreviewCursorChanger() { }
public slots:
    void SetBusyLoadingCursor();
    void RestoreCursor();
};

class QToolButton;
class QWebView;
class LocalHTMLLoadButtonEnabler : public QObject
{
    Q_OBJECT
private:
    QToolButton* m_refreshButton;
    QToolButton* m_stopButton;
    QWebView* m_webView;
public:
    LocalHTMLLoadButtonEnabler(QToolButton* refreshButton, QToolButton* stopButton,
                               QWebView* webView, QObject* parent);
    ~LocalHTMLLoadButtonEnabler() { }
private slots:
    void WebViewLoadStarted();
    void WebViewLoadFinished();
};

class LocalHTMLPreviewHandler : public FilePreviewHandler
{
private:
    LocalHTMLPreviewCursorChanger* m_cursorChanger;

public:
    LocalHTMLPreviewHandler();
    ~LocalHTMLPreviewHandler();

    // FilePreviewHandler interface
public:
    QString GetUniqueName();
    QStringList GetSupportedExtensions();
    FileCategory GetFilesCategory();

    QWidget* CreateAndFreeWidget(QWidget* parent);
    bool ClearAndSetDataToWidget(const QString& filePathName, QWidget* previewWidget);

};
