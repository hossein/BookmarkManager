#pragma
#include <QMainWindow>

#include "Config.h"
#include "DatabaseManager.h"
#include "FileManager.h"
#include "BookmarksFilteredByTagsSortProxyModel.h"

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
    BookmarksFilteredByTagsSortProxyModel filteredBookmarksModel;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnNew_clicked();
    void on_btnView_clicked();
    void on_btnEdit_clicked();
    void on_btnDelete_clicked();
    void on_tvBookmarks_activated(const QModelIndex &index);
    void tvBookmarksCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous);
    void tvBookmarksHeaderClicked(int logicalIndex);
    void lwTagsItemChanged(QListWidgetItem* item);

private:
    /// Initialization functions, to be called JUST ONCE for loading.
    void PreAssignModels();
    void LoadDatabaseAndUI();

    /// Master functions for data refresh and display.
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
        RA_SaveSelAndScrollAndFocus = RA_SaveSel | RA_SaveScrollPosAndFocus,
        RA_CustomSelAndSaveScrollAndFocus = RA_CustomSelect | RA_SaveScrollPosAndFocus,
        RA_CustomSelectAndFocus = RA_CustomSelect | RA_Focus,
        RA_SaveCheckState = 0x10, //Only for Tags
        RA_SaveSelAndScrollAndCheck = RA_SaveSelAndScroll | RA_SaveCheckState,
        RA_NoRefreshView = 0x20
    };
    void RefreshUIDataDisplay(bool rePopulateModels,
                              RefreshAction bookmarksAction = RA_None, long long selectBID = -1,
                              RefreshAction tagsAction = RA_None, long long selectTID = -1,
                              const QList<long long>& newTIDsToCheck = QList<long long>());
    void RefreshStatusLabels();

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

    enum TagCheckStateResult
    {
        TCSR_NoneChecked = 0,
        TCSR_SomeChecked = 1,
        TCSR_AllChecked = 2,
    };
    TagCheckStateResult m_allTagsChecked;
    void QueryAllTagsChecked();
    void UpdateAllTagsCheckBoxCheck();

    //The following function automatically Queries and updates checkbox, too.
    void CheckAllTags(Qt::CheckState checkState);

    QList<long long> GetCheckedTIDs();
    void RestoreCheckedTIDs(const QList<long long>& checkedTIDs,
                            const QList<long long>& newTIDsToCheck);
};
