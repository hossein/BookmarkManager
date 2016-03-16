#include "BMWebView.h"

#include <QContextMenuEvent>
#include <QWebPage>
#include <QWebFrame>

#include <QDesktopServices>

BMWebView::BMWebView(QWidget *parent) : QWebView(parent)
{

}

BMWebView::~BMWebView()
{

}

void BMWebView::contextMenuEvent(QContextMenuEvent* event)
{
    QWebHitTestResult result = page()->mainFrame()->hitTestContent(event->pos());
    m_lastLinkUrlForNewWindow = result.linkUrl();

    QWebView::contextMenuEvent(event);
}

QWebView* BMWebView::createWindow(QWebPage::WebWindowType type)
{
    QDesktopServices::openUrl(m_lastLinkUrlForNewWindow);
    return QWebView::createWindow(type); //Does nothing.
}
