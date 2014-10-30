#-------------------------------------------------
#
# Project created by QtCreator 2014-03-06T16:48:08
#
#-------------------------------------------------

QT       += core gui sql webkit

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BookmarkManager
TEMPLATE = app
LIBS += Shell32.lib User32.lib Version.lib

#To generate header file dependency:
#https://qt-project.org/forums/viewreply/82432/
LOCAL_INCLUDE_DIRS = $$_PRO_FILE_PWD_
DEPENDPATH *= $$(LOCAL_INCLUDE_DIRS)

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
    BookmarkRemovalManager.cpp

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
    BookmarkRemovalManager.h

FORMS    += MainWindow.ui \
    BookmarkEditDialog.ui \
    BookmarkViewDialog.ui \
    OpenWithDialog.ui \
    QuickBookmarkSelectDialog.ui \
    BookmarkExtraInfoAddEditDialog.ui

RESOURCES += \
    BookmarkManager.qrc
