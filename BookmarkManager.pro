#-------------------------------------------------
#
# Project created by QtCreator 2014-03-06T16:48:08
#
#-------------------------------------------------

QT       += core gui sql network webkit

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets webkitwidgets winextras

TARGET = BookmarkManager
TEMPLATE = app
LIBS += Shell32.lib User32.lib Version.lib

#Generate PDB files for the release build too.
#TODO: Temporary, or make sure this doesn't mess up with the release build.
QMAKE_CXXFLAGS_RELEASE += $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE += $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

#To generate header file dependency:
#https://qt-project.org/forums/viewreply/82432/
LOCAL_INCLUDE_DIRS = $$_PRO_FILE_PWD_ \
    $$_PRO_FILE_PWD_/BookmarkImporters \
    $$_PRO_FILE_PWD_/PreviewHandlers \
    $$_PRO_FILE_PWD_/qtsingleapplication
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
    BookmarkImporters/FirefoxBookmarkJSONFileParser.cpp \
    ImportedBookmarksPreviewDialog.cpp \
    BookmarkImporter.cpp \
    ImportedBookmarkProcessor.cpp \
    ImportedBookmarksProcessor.cpp \
    MHTSaver.cpp \
    SettingsManager.cpp \
    CtLogger.cpp \
    BMApplication.cpp \
    qtsingleapplication/qtlocalpeer.cpp \
    qtsingleapplication/qtsingleapplication.cpp \
    BookmarksSortFilterProxyModel.cpp \
    PreviewHandlers/BMWebView.cpp \
    BookmarkFolderManager.cpp \
    BookmarkFoldersView.cpp \
    BookmarkFolderEditDialog.cpp

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
    BookmarkImporters/ImportedEntity.h \
    ImportedBookmarksPreviewDialog.h \
    BookmarkImporter.h \
    ImportedBookmarkProcessor.h \
    ImportedBookmarksProcessor.h \
    MHTSaver.h \
    SettingsManager.h \
    ListWidgetWithEmptyPlaceholder.h \
    CtLogger.h \
    BMApplication.h \
    qtsingleapplication/qtlocalpeer.h \
    qtsingleapplication/qtsingleapplication.h \
    BookmarksSortFilterProxyModel.h \
    BookmarkFilter.h \
    PreviewHandlers/BMWebView.h \
    BookmarkFolderManager.h \
    BookmarkFoldersView.h \
    BookmarkFolderEditDialog.h

FORMS    += MainWindow.ui \
    BookmarkEditDialog.ui \
    BookmarkViewDialog.ui \
    OpenWithDialog.ui \
    QuickBookmarkSelectDialog.ui \
    BookmarkExtraInfoAddEditDialog.ui \
    ImportedBookmarksPreviewDialog.ui \
    BookmarkFolderEditDialog.ui

RESOURCES += \
    BookmarkManager.qrc
