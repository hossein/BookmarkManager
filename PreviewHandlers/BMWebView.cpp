#include "BMWebView.h"

#include <QContextMenuEvent>
#include <QWebEnginePage>

#include <QDesktopServices>

BMWebView::BMWebView(QWidget *parent) : QWebEngineView(parent)
{

}

BMWebView::~BMWebView()
{

}

/*void BMWebView::contextMenuEvent(QContextMenuEvent* event)
{
    QWebHitTestResult result = page()->mainFrame()->hitTestContent(event->pos());
    m_lastLinkUrlForNewWindow = result.linkUrl();

    QWebView::contextMenuEvent(event);
}*/

QWebEngineView* BMWebView::createWindow(QWebEnginePage::WebWindowType type)
{
    QDesktopServices::openUrl(m_lastLinkUrlForNewWindow);
    return QWebEngineView::createWindow(type); //Does nothing.
}
