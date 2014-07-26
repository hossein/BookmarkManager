#pragma
#include <QMainWindow>

#include "Config.h"
#include "DatabaseManager.h"
#include "FileManager.h"

#include <QModelIndex>

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;
    Config conf;
    DatabaseManager dbm;

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
    void RefreshTVBookmarksModelView();
    void NewBookmark();
    int GetSelectedBookmarkID();
    void ViewSelectedBookmark();
    void EditSelectedBookmark();
    void DeleteSelectedBookmark();
};
