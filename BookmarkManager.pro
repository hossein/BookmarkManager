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
    TagManager.cpp

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
    ISubManager.h

FORMS    += MainWindow.ui \
    BookmarkEditDialog.ui
