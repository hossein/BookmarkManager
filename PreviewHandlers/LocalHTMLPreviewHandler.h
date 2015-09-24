#pragma once
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
