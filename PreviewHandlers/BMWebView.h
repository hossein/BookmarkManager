#pragma once
#include <QWebView>

#include <QUrl>

class BMWebView : public QWebView
{
    Q_OBJECT

private:
    QUrl m_lastLinkUrlForNewWindow;

public:
    explicit BMWebView(QWidget *parent = 0);
    ~BMWebView();

    //From http://stackoverflow.com/questions/14804326/qt-pyqt-how-do-i-act-on-qwebview-qwebpages-open-in-new-window-action
protected:
    virtual void contextMenuEvent(QContextMenuEvent* event);
protected:
    virtual QWebView*createWindow(QWebPage::WebWindowType type);

};
