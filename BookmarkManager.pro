#-------------------------------------------------
#
# Project created by QtCreator 2014-03-06T16:48:08
#
#-------------------------------------------------

QT       += core gui sql network widgets webengine webenginewidgets winextras

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
    $$_PRO_FILE_PWD_/BookmarkFolders \
    $$_PRO_FILE_PWD_/BookmarkImporter \
    $$_PRO_FILE_PWD_/Bookmarks \
    $$_PRO_FILE_PWD_/Database \
    $$_PRO_FILE_PWD_/Files \
    $$_PRO_FILE_PWD_/FileViewer \
    $$_PRO_FILE_PWD_/PreviewHandlers \
    $$_PRO_FILE_PWD_/qtsingleapplication \
    $$_PRO_FILE_PWD_/Settings \
    $$_PRO_FILE_PWD_/Tags \
    $$_PRO_FILE_PWD_/Util
DEPENDPATH *= $$(LOCAL_INCLUDE_DIRS)

#Without this on Qt5 files inside the subfolders can't access the higher-level files directly.
#  Dangerous, but we set it so. Obviously not good for big projects. And we have to use <> then.
INCLUDEPATH += $$_PRO_FILE_PWD_

SOURCES += BMApplication.cpp \
    BookmarksBusinessLogic.cpp \
    main.cpp \
    MainWindow.cpp \
    BookmarkFolders/BookmarkFolderEditDialog.cpp \
    BookmarkFolders/BookmarkFolderManager.cpp \
    BookmarkFolders/BookmarkFoldersTreeWidget.cpp \
    BookmarkFolders/BookmarkFoldersView.cpp \
    BookmarkImporter/BookmarkImporter.cpp \
    BookmarkImporter/FirefoxBookmarkJSONFileParser.cpp \
    BookmarkImporter/ImportedBookmarkProcessor.cpp \
    BookmarkImporter/ImportedBookmarksPreviewDialog.cpp \
    BookmarkImporter/ImportedBookmarksProcessor.cpp \
    BookmarkImporter/MHTSaver.cpp \
    Bookmarks/BookmarkEditDialog.cpp \
    Bookmarks/BookmarkExtraInfoAddEditDialog.cpp \
    Bookmarks/BookmarkManager.cpp \
    Bookmarks/BookmarksSortFilterProxyModel.cpp \
    Bookmarks/BookmarksView.cpp \
    Bookmarks/BookmarkViewDialog.cpp \
    Bookmarks/FiveStarRatingWidget.cpp \
    Bookmarks/MergeConfirmationDialog.cpp \
    Bookmarks/QuickBookmarkSelectDialog.cpp \
    Database/DatabaseManager.cpp \
    Files/FileArchiveManager.cpp \
    Files/FileManager.cpp \
    Files/FileSandBoxManager.cpp \
    Files/IArchiveManager.cpp \
    FileViewer/AppListItemDelegate.cpp \
    FileViewer/FilePreviewerWidget.cpp \
    FileViewer/FileViewManager.cpp \
    FileViewer/OpenWithDialog.cpp \
    PreviewHandlers/BMWebView.cpp \
    PreviewHandlers/FilePreviewHandler.cpp \
    PreviewHandlers/ImagePreviewHandler.cpp \
    PreviewHandlers/LocalHTMLPreviewHandler.cpp \
    PreviewHandlers/TextPreviewHandler.cpp \
    qtsingleapplication/qtlocalpeer.cpp \
    qtsingleapplication/qtsingleapplication.cpp \
    Settings/SettingsDialog.cpp \
    Settings/SettingsManager.cpp \
    Tags/TagLineEdit.cpp \
    Tags/TagManager.cpp \
    Tags/TagsView.cpp \
    Util/CtLogger.cpp \
    Util/TransactionalFileOperator.cpp \
    Util/Util.cpp \
    Util/WindowSizeMemory.cpp \
    Util/WinFunctions.cpp

HEADERS += BMApplication.h \
    BookmarksBusinessLogic.h \
    Config.h \
    MainWindow.h \
    BookmarkFolders/BookmarkFolderEditDialog.h \
    BookmarkFolders/BookmarkFolderManager.h \
    BookmarkFolders/BookmarkFoldersTreeWidget.h \
    BookmarkFolders/BookmarkFoldersView.h \
    BookmarkImporter/BookmarkImporter.h \
    BookmarkImporter/FirefoxBookmarkJSONFileParser.h \
    BookmarkImporter/ImportedBookmarkProcessor.h \
    BookmarkImporter/ImportedBookmarksPreviewDialog.h \
    BookmarkImporter/ImportedBookmarksProcessor.h \
    BookmarkImporter/ImportedEntity.h \
    BookmarkImporter/MHTSaver.h \
    Bookmarks/BookmarkEditDialog.h \
    Bookmarks/BookmarkExtraInfoAddEditDialog.h \
    Bookmarks/BookmarkExtraInfoTypeChooser.h \
    Bookmarks/BookmarkFilter.h \
    Bookmarks/BookmarkManager.h \
    Bookmarks/BookmarksSortFilterProxyModel.h \
    Bookmarks/BookmarksView.h \
    Bookmarks/BookmarkViewDialog.h \
    Bookmarks/FiveStarRatingWidget.h \
    Bookmarks/MergeConfirmationDialog.h \
    Bookmarks/QuickBookmarkSelectDialog.h \
    Database/DatabaseManager.h \
    Database/IManager.h \
    Database/ISubManager.h \
    Files/FileArchiveManager.h \
    Files/FileManager.h \
    Files/FileSandBoxManager.h \
    Files/IArchiveManager.h \
    FileViewer/AppListItemDelegate.h \
    FileViewer/FilePreviewerWidget.h \
    FileViewer/FileViewManager.h \
    FileViewer/OpenWithDialog.h \
    PreviewHandlers/BMWebView.h \
    PreviewHandlers/FilePreviewHandler.h \
    PreviewHandlers/ImagePreviewHandler.h \
    PreviewHandlers/LocalHTMLPreviewHandler.h \
    PreviewHandlers/TextPreviewHandler.h \
    qtsingleapplication/qtlocalpeer.h \
    qtsingleapplication/qtsingleapplication.h \
    Settings/SettingsDialog.h \
    Settings/SettingsManager.h \
    Tags/TagLineEdit.h \
    Tags/TagManager.h \
    Tags/TagsView.h \
    Util/CtLogger.h \
    Util/ListWidgetWithEmptyPlaceholder.h \
    Util/RichRadioButton.h \
    Util/TransactionalFileOperator.h \
    Util/Util.h \
    Util/WindowSizeMemory.h \
    Util/WinFunctions.h

FORMS += MainWindow.ui \
    BookmarkFolders/BookmarkFolderEditDialog.ui \
    BookmarkImporter/ImportedBookmarksPreviewDialog.ui \
    Bookmarks/BookmarkEditDialog.ui \
    Bookmarks/BookmarkExtraInfoAddEditDialog.ui \
    Bookmarks/BookmarkViewDialog.ui \
    Bookmarks/MergeConfirmationDialog.ui \
    Bookmarks/QuickBookmarkSelectDialog.ui \
    FileViewer/OpenWithDialog.ui \
    Settings/SettingsDialog.ui

RESOURCES += \
    BookmarkManager.qrc
