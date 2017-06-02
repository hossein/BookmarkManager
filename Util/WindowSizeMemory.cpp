#include "WindowSizeMemory.h"

#include "Database/DatabaseManager.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QEvent>
#include <QMetaObject>
#include <QMetaMethod>
#include <QWidget>

WindowSizeMemory::WindowSizeMemory(QObject* parent, QWidget* watched, DatabaseManager* dbm,
        const QString& windowName, bool rememberPos, bool rememberSize,
        bool firstTimeResizeToDesktopSizeRatio, float firstTimeDesktopSizeRatio)
    : QObject(parent), watched(watched), dbm(dbm)
    , windowName(windowName), rememberPos(rememberPos), rememberSize(rememberSize)
    , firstTimeResizeToDesktopSizeRatio(firstTimeResizeToDesktopSizeRatio)
    , firstTimeDesktopSizeRatio(firstTimeDesktopSizeRatio)
    , S_X("WSM/" + windowName + "/x"), S_Y("WSM/" + windowName + "/y")
    , S_W("WSM/" + windowName + "/w"), S_H("WSM/" + windowName + "/h")
    , S_Max("WSM/" + windowName + "/max")
{
    savePosMethod = metaObject()->method(metaObject()->indexOfMethod("savePos()"));
    saveSizeMethod = metaObject()->method(metaObject()->indexOfMethod("saveSize()"));
}

void WindowSizeMemory::SetInitialSize()
{
    bool centerIt = false;
    int dWidth = QApplication::desktop()->availableGeometry().width();
    int dHeight = QApplication::desktop()->availableGeometry().height();

    //Set size and position upon setting up WindowSizeMemory.
    if (dbm->sets.HaveSetting(S_Max)) //Already run
    {
        if (rememberSize && dbm->sets.HaveSetting(S_W) && dbm->sets.HaveSetting(S_H))
            watched->resize(dbm->sets.GetSetting(S_W, 0), dbm->sets.GetSetting(S_H, 0));
        if (rememberPos && dbm->sets.HaveSetting(S_X) && dbm->sets.HaveSetting(S_Y))
            watched->move(dbm->sets.GetSetting(S_X, 0), dbm->sets.GetSetting(S_Y, 0));

        if (dbm->sets.GetSetting(S_Max, false))
            watched->setWindowState(Qt::WindowMaximized);
        else if (watched->x() < 0 || watched->x() >= dWidth || watched->y() < 0 || watched->y() >= dHeight)
            centerIt = true;
    }
    else if (firstTimeResizeToDesktopSizeRatio) //First time running, set size
    {
        watched->resize((int)(dWidth * firstTimeDesktopSizeRatio), dHeight * firstTimeDesktopSizeRatio);
        centerIt = true;
    }
    else //First time running, do nothing
    {

    }

    if (centerIt)
    {
        QRect geom = watched->geometry();
        geom.moveCenter(QApplication::desktop()->availableGeometry().center());
        watched->setGeometry(geom);
    }

    m_x = watched->x();
    m_y = watched->y();
    m_w = watched->width();
    m_h = watched->height();
}

WindowSizeMemory* WindowSizeMemory::SetWindowSizeMemory(QObject* parent, QWidget* watched, DatabaseManager* dbm,
        const QString& windowName, bool rememberPos, bool rememberSize,
        bool firstTimeResizeToDesktopSizeRatio, float firstTimeDesktopSizeRatio)
{
    WindowSizeMemory* wsm = new WindowSizeMemory(
                parent, watched, dbm, windowName, rememberPos, rememberSize,
                firstTimeResizeToDesktopSizeRatio, firstTimeDesktopSizeRatio);
    watched->installEventFilter(wsm);
    wsm->SetInitialSize();
    return wsm;
}

bool WindowSizeMemory::eventFilter(QObject* obj, QEvent* event)
{
    switch (event->type())
    {
    case QEvent::Move:
    {
        if (rememberPos && !watched->isMaximized())
            savePosMethod.invoke(this, Qt::QueuedConnection);
        return false;
    }
    case QEvent::Resize:
    {
        if (rememberSize && !watched->isMaximized())
            saveSizeMethod.invoke(this, Qt::QueuedConnection);
        return false;
    }
    case QEvent::Close:
    {
        dbm->sets.SetSetting(S_X, m_x);
        dbm->sets.SetSetting(S_Y, m_y);
        dbm->sets.SetSetting(S_W, m_w);
        dbm->sets.SetSetting(S_H, m_h);
        dbm->sets.SetSetting(S_Max, watched->isMaximized());
        return false;
    }
    default:
    {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
    }
}

void WindowSizeMemory::savePos()
{
    if (watched->isMaximized())
        return;
    m_x = watched->x();
    m_y = watched->y();
}

void WindowSizeMemory::saveSize()
{
    if (watched->isMaximized())
        return;
    m_w = watched->width();
    m_h = watched->height();
}
