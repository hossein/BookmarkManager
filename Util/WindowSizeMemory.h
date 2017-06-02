#pragma once
#include <QObject>
#include <QMetaMethod>

class DatabaseManager;

/// This class can be used to remember position and size of a window or dialog. Window's Maximized
///   status is always remembered and restored; but remembering pos and/or size is optional.
class WindowSizeMemory : public QObject
{
    Q_OBJECT

private:
    QWidget* watched;
    DatabaseManager* dbm;
    QString windowName;
    bool rememberPos;
    bool rememberSize;
    bool firstTimeResizeToDesktopSizeRatio;
    float firstTimeDesktopSizeRatio;

    const QString S_X;
    const QString S_Y;
    const QString S_W;
    const QString S_H;
    const QString S_Max;

    int m_x;
    int m_y;
    int m_w;
    int m_h;

    //We invoke, because when we maximize the window, before window->isMaximized() returns true
    //we receive both pos and size events with maximized window's location. This fixes it.
    QMetaMethod savePosMethod;
    QMetaMethod saveSizeMethod;

private:
    explicit WindowSizeMemory(
            QObject* parent, QWidget* watched, DatabaseManager* dbm,
            const QString& windowName, bool rememberPos, bool rememberSize,
            bool firstTimeResizeToDesktopSizeRatio, float firstTimeDesktopSizeRatio);

    void SetInitialSize();

public:
    static WindowSizeMemory* SetWindowSizeMemory(
            QObject* parent, QWidget* watched, DatabaseManager* dbm,
            const QString& windowName, bool rememberPos, bool rememberSize,
            bool firstTimeResizeToDesktopSizeRatio, float firstTimeDesktopSizeRatio);

protected:
    virtual bool eventFilter(QObject* obj, QEvent* event);

private slots:
    void savePos();
    void saveSize();
};
