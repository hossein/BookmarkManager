﻿#pragma
#include <QMainWindow>

#include "Config.h"
#include "DatabaseManager.h"
#include "FileManager.h"

#include <QHash>
#include <QModelIndex>

class QListWidgetItem;
namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;
    Config conf;
    DatabaseManager dbm;
    QHash<long long, QListWidgetItem*> tagItems;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnNew_clicked();
    void on_btnEdit_clicked();
    void on_btnDelete_clicked();
    void on_tvBookmarks_activated(const QModelIndex &index);
    void tvBookmarksCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous);

private:
    void LoadDatabaseAndUI();

    enum RefreshAction
    {
        RA_None = 0x00,
        RA_SaveSel = 0x01,
        RA_SaveScrollPos = 0x02,
        RA_SaveSelAndScroll = RA_SaveSel | RA_SaveScrollPos,
        RA_CustomSelect = 0x04,
        RA_Focus = 0x08, //Make the selection vivid blue! Instead of gray.
        RA_SaveSelAndFocus = RA_SaveSel | RA_Focus,
        RA_SaveScrollPosAndFocus = RA_SaveScrollPos | RA_Focus,
        RA_SaveSelAndScrollAndFocus = RA_SaveSelAndScroll | RA_Focus,
        RA_CustomSelectAndFocus = RA_CustomSelect | RA_Focus
    };
    void RefreshUIDataDisplay(RefreshAction bookmarksAction = RA_None, long long selectBID = -1,
                              RefreshAction tagsAction = RA_None, long long selectTID = -1);


    void RefreshTVBookmarksModelView();
    void NewBookmark();
    long long GetSelectedBookmarkID();
    void SelectBookmarkWithID(long long bookmarkId);
    void ViewSelectedBookmark();
    void EditSelectedBookmark();
    void DeleteSelectedBookmark();

    void RefreshTagsDisplay();
    long long GetSelectedTagID();
    void SelectTagWithID(long long tagId);
};
