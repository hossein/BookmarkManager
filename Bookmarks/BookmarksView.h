#pragma once
#include <QWidget>

#include "Config.h"

class QModelIndex;
class QAbstractItemModel;
class QItemSelection;
class QScrollBar;
class QTableView;

struct BookmarkFilter;
class BookmarksSortFilterProxyModel;
class BookmarkFoldersView;
class DatabaseManager;
class TagsView;

/// To make this class work, caller needs to create an instance AND call `Initialize`.
class BookmarksView : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool shrinkHeight READ shrinkHeight WRITE setShrinkHeight)

private:
    QWidget* dialogParent;
    QTableView* tvBookmarks;
    DatabaseManager* dbm;
    BookmarksSortFilterProxyModel* filteredBookmarksModel;
    int sortNextLogicalIndex;
    Qt::SortOrder sortNextOrder;
    bool m_shrinkHeight;

public:
    enum ListMode
    {
        //TODO: In NameOnly and Limited modes, (used e.g in EditDlg and ViewDlg's related bookmarks)
        //  must pop up their view dialog, AND they MUST show their Office-style brief information
        //  EVERYWHERE; super useful. Also we have to use suitable tooltips WHICH SHOW URL too for
        //  whenever user hovers any of them.
        LM_NameOnlyDisplayWithoutHeaders,
        LM_LimitedDisplayWithoutHeaders,
        LM_LimitedDisplayWithHeaders,
        LM_FullInformationAndEdit,
    };
private:
    ListMode m_listMode;

public:
    explicit BookmarksView(QWidget* parent = 0);

    /// This class MUST be initialized after db is ready by calling this function.
    void Initialize(DatabaseManager* dbm, ListMode listMode, QAbstractItemModel* model);

    //QWidget interface
protected:
    virtual void focusInEvent(QFocusEvent* event);

    //Action and Information Functions
public:
    void RefreshUIDataDisplay(bool rePopulateModels, const BookmarkFilter& bfilter,
                              UIDDRefreshAction refreshAction = RA_None,
                              const QList<long long>& selectedBIDs = QList<long long>());

    QStringList GetSelectedBookmarkNames() const;
    QList<long long> GetSelectedBookmarkIDs() const;
    void SelectBookmarksWithIDs(const QList<long long>& bookmarkIds);

    //Passes to BookmarksSortFilterProxyModel
    bool SetFilter(const BookmarkFilter& filter, bool forceReset);
    //This replaces `ResetHeadersAndSort()`. It just adjusts column widths.
    void RefreshView();

    int GetTotalBookmarksCount() const;
    int GetDisplayedBookmarksCount() const;

    void ScrollToBottom();

    //ShrinkHeight property
public:
    bool shrinkHeight() const        { return m_shrinkHeight;  }
public slots:
    void setShrinkHeight(bool value) { m_shrinkHeight = value; }

    //Private slots to make things work.
private slots:
    void modelLayoutChanged();
    void tvBookmarksActivated(const QModelIndex &index);
    void tvBookmarksSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void tvBookmarksHeaderPressed(int logicalIndex);
    void tvBookmarksHeaderClicked(int logicalIndex);

signals:
    void activated(long long BID);
    void selectionChanged(const QList<long long>& selectedBIDs);
};
