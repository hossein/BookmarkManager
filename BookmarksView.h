#pragma once
#include <QWidget>

class QModelIndex;
class QAbstractItemModel;
class QItemSelectionModel;
class QScrollBar;
class QTableView;

class Config;
class DatabaseManager;
class BookmarksFilteredByTagsSortProxyModel;

/// To make this class work, caller needs to create an instance AND call all the following
/// three functions (unless ResetHeadersAndSort becomes automatic):
/// Initialize
/// setModel
/// ResetHeadersAndSort
class BookmarksView : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool shrinkHeight READ shrinkHeight WRITE setShrinkHeight)

private:
    QWidget* dialogParent;
    QTableView* tvBookmarks;
    DatabaseManager* dbm;
    Config* conf;
    BookmarksFilteredByTagsSortProxyModel* filteredBookmarksModel;
    int sortNextLogicalIndex;
    Qt::SortOrder sortNextOrder;
    bool m_shrinkHeight;

public:
    enum ListMode
    {
        LM_NameOnlyDisplayWithoutHeaders, //TODO: Use suitable tooltips WHICH SHOW URL in this mode. For BMViewDlg.
        LM_LimitedDisplayWithoutHeaders,
        LM_LimitedDisplayWithHeaders,
        LM_FullInformationAndEdit,
    };
private:
    ListMode m_listMode;

public:
    explicit BookmarksView(QWidget* parent = 0);

    /// This class MUST be initialized by calling this function.
    void Initialize(DatabaseManager* dbm, Config* conf, ListMode listMode);

    //Action and Information Functions
public:
    QString GetSelectedBookmarkName() const;
    long long GetSelectedBookmarkID() const;
    void SelectBookmarkWithID(long long bookmarkId);

    void ClearFilters();
    bool FilterSpecificBookmarkIDs(const QList<long long>& BIDs);
    bool FilterSpecificTagIDs(const QSet<long long>& tagIDs);

    int GetTotalBookmarksCount() const;
    int GetDisplayedBookmarksCount() const;

    //Imitate QTableView behaviour
public:
    void setModel(QAbstractItemModel* model);

    //Imitate QTableView behaviour, but NOTE: probably can be changed.
public:
    QItemSelectionModel* selectionModel() const;
    void setCurrentIndex(const QModelIndex& index);
    QScrollBar* horizontalScrollBar() const;
    QScrollBar* verticalScrollBar() const;

    //ShrinkHeight property
public:
    bool shrinkHeight() const        { return m_shrinkHeight;  }
public slots:
    void setShrinkHeight(bool value) { m_shrinkHeight = value; }

public slots:
    /// Must be called after each model update, sort, etc. These were formerly at the bottom of
    /// MainWindow::RefreshTVBookmarksModelView. Dunno if we can make this automatic. I think we should
    /// connect BOTH dataChanged (obvious) and layoutChanged (filter, sort). AND also on setModel,
    /// as user initially calls it.
    void ResetHeadersAndSort();

    //Private slots to make things work.
private slots:
    void modelLayoutChanged();
    void tvBookmarksActivated(const QModelIndex &index);
    void tvBookmarksCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous);
    void tvBookmarksHeaderPressed(int logicalIndex);
    void tvBookmarksHeaderClicked(int logicalIndex);

signals:
    void activated(long long BID);
    void currentRowChanged(long long currentBID, long long previousBID);
};
