#-------------------------------------------------
#
# Project created by QtCreator 2014-03-06T16:48:08
#
#-------------------------------------------------

QT       += core gui sql webkit

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets webkitwidgets winextras

TARGET = BookmarkManager
TEMPLATE = app
LIBS += Shell32.lib User32.lib Version.lib

#To generate header file dependency:
#https://qt-project.org/forums/viewreply/82432/
LOCAL_INCLUDE_DIRS = $$_PRO_FILE_PWD_ \
    $$_PRO_FILE_PWD_/PreviewHandlers
DEPENDPATH *= $$(LOCAL_INCLUDE_DIRS)

#Without this on Qt5 files inside the subfolders can't access the higher-level files directly.
#  Dangerous, but we set it so. Obviously not good for big projects. And we have to use <> then.
INCLUDEPATH += $$_PRO_FILE_PWD_

SOURCES += main.cpp\
        MainWindow.cpp \
    DatabaseManager.cpp \
    BookmarkEditDialog.cpp \
    TagLineEdit.cpp \
    WinFunctions.cpp \
    BookmarkManager.cpp \
    FileManager.cpp \
    TagManager.cpp \
    Util.cpp \
    TransactionalFileOperator.cpp \
    FileViewManager.cpp \
    BookmarksFilteredByTagsSortProxyModel.cpp \
    FiveStarRatingWidget.cpp \
    BookmarkViewDialog.cpp \
    FilePreviewerWidget.cpp \
    PreviewHandlers/LocalHTMLPreviewHandler.cpp \
    PreviewHandlers/FilePreviewHandler.cpp \
    PreviewHandlers/ImagePreviewHandler.cpp \
    PreviewHandlers/TextPreviewHandler.cpp \
    OpenWithDialog.cpp \
    AppListItemDelegate.cpp \
    FileArchiveManager.cpp \
    FileSandBoxManager.cpp \
    IArchiveManager.cpp \
    BookmarksView.cpp \
    QuickBookmarkSelectDialog.cpp \
    BookmarkExtraInfoAddEditDialog.cpp \
    BookmarksBusinessLogic.cpp \
    BookmarkImporters/FirefoxBookmarkJSONFileParser.cpp

HEADERS  += MainWindow.h \
    DatabaseManager.h \
    Config.h \
    BookmarkEditDialog.h \
    TagLineEdit.h \
    WinFunctions.h \
    BookmarkManager.h \
    IManager.h \
    FileManager.h \
    TagManager.h \
    ISubManager.h \
    Util.h \
    TransactionalFileOperator.h \
    FileViewManager.h \
    BookmarksFilteredByTagsSortProxyModel.h \
    FiveStarRatingWidget.h \
    BookmarkViewDialog.h \
    FilePreviewerWidget.h \
    PreviewHandlers/FilePreviewHandler.h \
    PreviewHandlers/LocalHTMLPreviewHandler.h \
    PreviewHandlers/ImagePreviewHandler.h \
    PreviewHandlers/TextPreviewHandler.h \
    OpenWithDialog.h \
    AppListItemDelegate.h \
    FileArchiveManager.h \
    IArchiveManager.h \
    FileSandBoxManager.h \
    BookmarksView.h \
    QuickBookmarkSelectDialog.h \
    BookmarkExtraInfoAddEditDialog.h \
    BookmarkExtraInfoTypeChooser.h \
    BookmarksBusinessLogic.h \
    BookmarkImporters/FirefoxBookmarkJSONFileParser.h \
    BookmarkImporters/ImportedEntity.h

FORMS    += MainWindow.ui \
    BookmarkEditDialog.ui \
    BookmarkViewDialog.ui \
    OpenWithDialog.ui \
    QuickBookmarkSelectDialog.ui \
    BookmarkExtraInfoAddEditDialog.ui

RESOURCES += \
    BookmarkManager.qrc
