#-------------------------------------------------
#
# Project created by QtCreator 2014-03-06T16:48:08
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BookmarkManager
TEMPLATE = app
LIBS += Shell32.lib

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
    BookmarkViewDialog.cpp

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
    BookmarkViewDialog.h

FORMS    += MainWindow.ui \
    BookmarkEditDialog.ui \
    BookmarkViewDialog.ui

RESOURCES += \
    BookmarkManager.qrc
