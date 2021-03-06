#include "BookmarksView.h"

#include "BookmarksSortFilterProxyModel.h"
#include "BookmarkFolders/BookmarkFoldersView.h"
#include "Database/DatabaseManager.h"
#include "Tags/TagsView.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QTableView>

BookmarksView::BookmarksView(QWidget *parent)
    : QWidget(parent)
    , dialogParent(parent), tvBookmarks(NULL), dbm(NULL), filteredBookmarksModel(NULL)
{
    //Initialize this here to protect from some crashes
    tvBookmarks = new QTableView(this);
    tvBookmarks->setDragEnabled(true); // < v Either one was enough
    tvBookmarks->setDragDropMode(QAbstractItemView::DragOnly);
    tvBookmarks->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tvBookmarks->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tvBookmarks->setSelectionBehavior(QAbstractItemView::SelectRows);
    tvBookmarks->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tvBookmarks->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    tvBookmarks->setWordWrap(false);
}

void BookmarksView::Initialize(DatabaseManager* dbm, ListMode listMode, QAbstractItemModel* model)
{
    //Class members
    this->dbm = dbm;
    this->m_listMode = listMode;
    this->m_shrinkHeight = false;

    //UI
    QHeaderView* hh = tvBookmarks->horizontalHeader();
    QHeaderView* vh = tvBookmarks->verticalHeader();
    hh->setVisible(listMode >= LM_LimitedDisplayWithHeaders);
    hh->setHighlightSections(false);
    vh->setVisible(false);
    vh->setHighlightSections(false);

    QHBoxLayout* myLayout = new QHBoxLayout();
    myLayout->addWidget(tvBookmarks);
    myLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(myLayout);

    //First time connections, sort and activation
    //tvBookmarks->setSortingEnabled(true); Leave it false, we're managing header clicks manually.
    connect(tvBookmarks, SIGNAL(activated(QModelIndex)), this, SLOT(tvBookmarksActivated(QModelIndex)));
    connect(hh, SIGNAL(sectionPressed(int)), this, SLOT(tvBookmarksHeaderPressed(int)));
    connect(hh, SIGNAL(sectionClicked(int)), this, SLOT(tvBookmarksHeaderClicked(int)));

    //Models
    filteredBookmarksModel = new BookmarksSortFilterProxyModel(dbm, dialogParent, this);
    connect(filteredBookmarksModel, SIGNAL(layoutChanged()), this, SLOT(modelLayoutChanged()));

    filteredBookmarksModel->setSourceModel(model);
    tvBookmarks->setModel(filteredBookmarksModel);

    //Must hide the sections AFTER setting the model.
    BookmarkManager::BookmarkIndexes const& bidx = dbm->bms.bidx;
    if (hh->count() > 0) //This can happen on database errors.
    {
        hh->hideSection(bidx.BID);
        hh->hideSection(bidx.FOID);
        hh->hideSection(bidx.Desc);
        hh->hideSection(bidx.DefBFID);
        hh->hideSection(bidx.AddDate);

        if (m_listMode <= LM_LimitedDisplayWithHeaders)
            hh->hideSection(bidx.Rating);

        if (m_listMode <= LM_NameOnlyDisplayWithoutHeaders)
            hh->hideSection(bidx.URLs);

        hh->setSectionResizeMode(bidx.Name, QHeaderView::Stretch);

        hh->resizeSection(bidx.URLs, 200);
        //TODO [handle]: How to show tags? hh->resizeSection(dbm.bidx.Tags, 100);
        //TODO: We also need to show folders, e.g in 'Linked Bookmarks' or 'All Bookmarks' views.
        hh->resizeSection(bidx.Rating, 50);
    }

    ///Initial sort, let it be unsorted
    ///Qt::SortOrder sortOrder = hh->sortIndicatorOrder();
    ///int sortColumn = hh->sortIndicatorSection();
    ///filteredBookmarksModel->sort(sortColumn, sortOrder);
    //Wrong ways to sort:
    //tvBookmarks->sortByColumn(sortColumn);
    //hh->setSortIndicator(sortColumn, sortOrder);

    vh->setSectionResizeMode(QHeaderView::ResizeToContents); //Disable changing row height.

    //Need to connect everytime we change the model.
    connect(tvBookmarks->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(tvBookmarksSelectionChanged(QItemSelection,QItemSelection)));
}

void BookmarksView::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
    tvBookmarks->setFocus();
}

void BookmarksView::RefreshUIDataDisplay(bool rePopulateModels, const BookmarkFilter& bfilter,
                                         UIDDRefreshAction refreshAction, const QList<long long>& selectedBIDs)
{
    int hBScrollPos = 0, vBScrollPos = 0;

    //Save required things.
    //This is required to be a persistent model for saving selections after filtering.
    QPersistentModelIndex selectedBRowIndex;
    if (refreshAction & RA_SaveSel) //[SavingSelectedBookmarkAndTag]
        if (tvBookmarks->selectionModel()->selectedIndexes().size() > 0)
            //!selectedBRow = tvBookmarks->selectionModel()->selectedIndexes()[0].row();
            selectedBRowIndex = tvBookmarks->selectionModel()->selectedIndexes()[0];
    if (refreshAction & RA_SaveScrollPos)
    {
        if (tvBookmarks->horizontalScrollBar() != NULL)
            hBScrollPos = tvBookmarks->horizontalScrollBar()->value();
        if (tvBookmarks->verticalScrollBar() != NULL)
            vBScrollPos = tvBookmarks->verticalScrollBar()->value();
    }

    if (rePopulateModels)
        dbm->bms.PopulateModelsAndInternalTables();

    //Technically we only need to forceResetFilter only when adding and deleting a bookmark, so we
    //  could have e.g a new action called `RA_ForceResetFilter`; but for simplicity we force
    //  resetting the filter on every change, including the unneeded startup and editing actions.
    if (!(refreshAction & RA_NoRefreshView))
    {
        SetFilter(bfilter, rePopulateModels);
        RefreshView();
    }

    //Pour out saved selections, scrolls, etc.
    //Note on 20141009:
    //  To make selection compatible with sorting, we couldn't just select rows. We use ModelIndexes.
    //  With the new Bookmark selection saving method, when filtering by tags, instead of selecting
    //  something ourselves, we could not select anything and let the TableView do it itself!
    //  But if a bookmark is selected and then its tag is deselected, the bookmark disappears.
    //  If TableWidget manages selections, it just selects the nearest bookmark, but with our custom
    //  selecting, selection is cleared and we prefer it.
    //Important: This selecting is only useful for filtering, as all other actions manage the
    //  selections themselves, but Edit must manually select now, too; as model indices (even
    //  persistent ones) are invalid after a model reset.
    if (refreshAction & RA_SaveSel)
        tvBookmarks->setCurrentIndex(selectedBRowIndex);
        //if (selectedBRow != -1)
            //!tvBookmarks->setCurrentIndex(filteredBookmarksModel.index(selectedBRow, 0));

    if (refreshAction & RA_SaveScrollPos)
    {
        if (tvBookmarks->horizontalScrollBar() != NULL)
            tvBookmarks->horizontalScrollBar()->setValue(hBScrollPos);
        if (tvBookmarks->verticalScrollBar() != NULL)
            tvBookmarks->verticalScrollBar()->setValue(vBScrollPos);
    }

    //[RestoringScrollPositionProceedsCustomSelection]
    if (refreshAction & RA_CustomSelect)
        if (!selectedBIDs.empty())
            SelectBookmarksWithIDs(selectedBIDs);

    //Focusing comes last anyway
    if (refreshAction & RA_Focus)
        tvBookmarks->setFocus();
}

QStringList BookmarksView::GetSelectedBookmarkNames() const
{
    if (!filteredBookmarksModel)
        return QStringList();

    QStringList selectedNames;
    QModelIndexList selectedIndices = tvBookmarks->selectionModel()->selectedRows(dbm->bms.bidx.Name);
    foreach (const QModelIndex& index, selectedIndices)
        selectedNames.append(index.data().toString()); //See [BV Accessing Data]
    return selectedNames;
}

QList<long long> BookmarksView::GetSelectedBookmarkIDs() const
{
    if (!filteredBookmarksModel)
        return QList<long long>();

    QList<long long> selectedBIds;
    QModelIndexList selectedIndices = tvBookmarks->selectionModel()->selectedRows(dbm->bms.bidx.BID);
    foreach (const QModelIndex& index, selectedIndices)
        selectedBIds.append(index.data().toLongLong()); //See [BV Accessing Data]
    return selectedBIds;
}

void BookmarksView::SelectBookmarksWithIDs(const QList<long long>& bookmarkIds)
{
    if (!filteredBookmarksModel)
        return;

    QModelIndexList bookmarkMatches;
    foreach (long long BID, bookmarkIds)
    {
        QModelIndexList matches = filteredBookmarksModel->match(
                    filteredBookmarksModel->index(0, dbm->bms.bidx.BID), Qt::DisplayRole,
                    BID, 1, Qt::MatchExactly);
        if (matches.length() == 1) //May not be found for some reason, e.g filtered out.
            bookmarkMatches.append(matches[0]);
    }

    QItemSelection itemSelection;
    QItemSelectionModel* selModel = tvBookmarks->selectionModel();
    //selModel->clear();
    foreach (const QModelIndex& index, bookmarkMatches)
        itemSelection.select(index, index);
    selModel->select(itemSelection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    //TODO: This does not ensure visible on bookmark adding and dragging bms onto folders and on import.
    if (!bookmarkMatches.empty())
        tvBookmarks->scrollTo(bookmarkMatches[0], QAbstractItemView::EnsureVisible);
}

bool BookmarksView::SetFilter(const BookmarkFilter& filter, bool forceReset)
{
    if (!filteredBookmarksModel)
        return false;
    return filteredBookmarksModel->SetFilter(filter, forceReset);
}

void BookmarksView::RefreshView()
{
    //Currently this does nothing. Maybe the point is to remove it because things are automatic?
}

int BookmarksView::GetTotalBookmarksCount() const
{
    if (!filteredBookmarksModel)
        return 0;
    return filteredBookmarksModel->sourceModel()->rowCount();
}

int BookmarksView::GetDisplayedBookmarksCount() const
{
    if (!filteredBookmarksModel)
        return 0;
    return filteredBookmarksModel->rowCount();
}

void BookmarksView::ScrollToBottom()
{
    tvBookmarks->verticalScrollBar()->setValue(tvBookmarks->verticalScrollBar()->maximum());
}

void BookmarksView::modelLayoutChanged()
{
    //Note: This caused the 'Name' column stretch to be gone when user clicked headers for sort
    //  (see b75a9ea4). On adding/editing bookmarks the stretch returned back again correctly!
    //  Don't know what was their usage.
    //tvBookmarks->resizeColumnsToContents();
    //tvBookmarks->resizeRowsToContents();

    if (m_shrinkHeight)
    {
        //Calculate the minimum required height.
        int hackedSuitableHeightForTvBookmarks =
                tvBookmarks->frameWidth() * 2 +
                //The row height includes the grid; no need to `rowHeight+1`.
                tvBookmarks->rowHeight(0) * GetDisplayedBookmarksCount();

        if (tvBookmarks->horizontalHeader()->isVisible())
            hackedSuitableHeightForTvBookmarks += tvBookmarks->horizontalHeader()->sizeHint().height();

        //Note: Although the hscrollbar may ruin the size it it's needed to disregard it; but it seems
        //  that it doesn't show up automatically, it switched to text eliding (`...`).
        this->setFixedHeight(hackedSuitableHeightForTvBookmarks);
    }
}

void BookmarksView::tvBookmarksActivated(const QModelIndex& index)
{
    int row = index.row();
    QModelIndex indexOfBID = filteredBookmarksModel->index(row, dbm->bms.bidx.BID);
    long long BID = filteredBookmarksModel->data(indexOfBID).toLongLong();
    emit activated(BID);
}

void BookmarksView::tvBookmarksSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);
    QList<long long> selectedBIDs;

    //QModelIndexList selectedIndices = selected.indexes(); <-- This returned every single cell
    QModelIndexList selectedIndices = tvBookmarks->selectionModel()->selectedRows(dbm->bms.bidx.BID);
    foreach (const QModelIndex& index, selectedIndices)
    {
        //[BV Accessing Data]:
        //  The following two seem to be equivalent here. So some places in this class that use the second form seem to
        //  be redundant and unnecessarily verbose.
        selectedBIDs.append(index.data().toLongLong());
        //selectedBIDs.append(filteredBookmarksModel->index(index.row(), dbm->bms.bidx.BID).data().toLongLong());
    }

    emit selectionChanged(selectedBIDs);
}

void BookmarksView::tvBookmarksHeaderPressed(int logicalIndex)
{
    //We use this slot to allow the user to clear the sort and to restore the default order,
    //  i.e BID which must be roughly equivalent to adding date, or adding order in browsers.
    //We can't use the section clicked slot only, as its values are AFTER Qt changes the sort
    //  order itself. We use both pressed and clicked slots to acheive what we want.

    QHeaderView* hh = tvBookmarks->horizontalHeader();
    int currentSectionIndex = (hh->isSortIndicatorShown() ? hh->sortIndicatorSection() : -1);
    Qt::SortOrder currentSortOrder = hh->sortIndicatorOrder();

    if (logicalIndex == currentSectionIndex) //logicalIndex is never -1 here.
    {
        if (currentSortOrder == Qt::AscendingOrder)
        {
            sortNextLogicalIndex = logicalIndex;
            sortNextOrder = Qt::DescendingOrder;
        }
        else
        {
            sortNextLogicalIndex = -1;
        }
    }
    else
    {
        sortNextLogicalIndex = logicalIndex;
        sortNextOrder = Qt::AscendingOrder;
    }
}

void BookmarksView::tvBookmarksHeaderClicked(int logicalIndex)
{
    Q_UNUSED(logicalIndex);

    //TODO [handle]: Do we need BOTH indicator setting and sorting the proxy model?
    QHeaderView* hh = tvBookmarks->horizontalHeader();
    hh->setSortIndicatorShown(sortNextLogicalIndex != -1);
    hh->setSortIndicator(sortNextLogicalIndex, sortNextOrder);

    if (sortNextLogicalIndex == -1)
        filteredBookmarksModel->sort(dbm->bms.bidx.BID, Qt::AscendingOrder);
    else
        filteredBookmarksModel->sort(logicalIndex, sortNextOrder);
}
