#pragma once
#include <QMainWindow>

#include "Config.h"
#include "Database/DatabaseManager.h"
#include "Files/FileManager.h"

#include <QHash>

struct BookmarkFilter;
struct ImportedEntityList;
class QListWidgetItem;
namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;
    Config conf;
    DatabaseManager dbm;
    bool m_shouldExit;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool shouldExit() const;

private slots:
    //// Private slots responding to UI ///////////////////////////////////////////////////////////
    void on_btnNew_clicked();
    void on_btnMerge_clicked();
    void on_btnView_clicked();
    void on_btnEdit_clicked();
    void on_btnDelete_clicked();
    void bvActivated(long long BID);
    void bvSelectionChanged(const QList<long long>& selectedBIDs);
    void tfCurrentFolderChanged(long long FOID);
    void tfRequestMoveBookmarksToFolder(const QList<long long>& BIDs, long long FOID);
    void tvTagSelectionChanged();
    void leSearchTextChanged(const QString& text);
    void chkSearchRegExpToggled(bool checked);

    void on_action_importFirefoxBookmarks_triggered();
    void on_actionImportFirefoxBookmarksJSONfile_triggered();
    void on_actionImportUrlsAsBookmarks_triggered();
    void on_actionGetMHT_triggered();
    void on_actionSettings_triggered();

private:
    /// Master functions for data refresh and display /////////////////////////////////////////////
    void RefreshUIDataDisplay(bool rePopulateModels,
                              UIDDRefreshAction bookmarksAction = RA_None, const QList<long long>& selectBIDs = QList<long long>(),
                              UIDDRefreshAction tagsAction = RA_None, long long selectTID = -1,
                              const QList<long long>& newTIDsToCheck = QList<long long>());
    void GetBookmarkFilter(BookmarkFilter& bfilter);
    void RefreshStatusLabels();

    void NewBookmark();
    void MergeSelectedBookmarks();
    void ViewSelectedBookmark();
    void EditSelectedBookmark();
    void DeleteSelectedBookmarks();

    //// Bookmark importing ///////////////////////////////////////////////////////////////////////
    void ImportURLs(const QStringList& urls, long long importFOID);
    void ImportFirefoxJSONFile(const QString& jsonFilePath);
    void ImportBookmarks(ImportedEntityList& elist);
};
