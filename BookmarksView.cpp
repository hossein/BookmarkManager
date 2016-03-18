#include "BookmarksView.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableView>

#include "BookmarksSortFilterProxyModel.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#   define setSectionResizeMode setResizeMode
#endif

BookmarksView::BookmarksView(QWidget *parent)
    : QWidget(parent)
    , dialogParent(parent), tvBookmarks(NULL), dbm(NULL), filteredBookmarksModel(NULL)
{
    //Initialize this here to protect from some crashes
    tvBookmarks = new QTableView(this);
    tvBookmarks->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tvBookmarks->setSelectionMode(QAbstractItemView::SingleSelection);
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
            hh->hideSection(bidx.URL);

        hh->setSectionResizeMode(bidx.Name, QHeaderView::Stretch);

        hh->resizeSection(bidx.URL, 200);
        //TODO [handle]: How to show tags? hh->resizeSection(dbm.bidx.Tags, 100);
        hh->resizeSection(bidx.Rating, 50);
    }

    ///Initial sort, let it be unsorted
    ///Qt::SortOrder sortOrder = hh->sortIndicatorOrder();
    ///int sortColumn = hh->sortIndicatorSection();
    ///filteredBookmarksModel->sort(sortColumn, sortOrder);
    //Wrong ways to sort:
    //ui->tvBookmarks->sortByColumn(sortColumn);
    //hh->setSortIndicator(sortColumn, sortOrder);

    vh->setSectionResizeMode(QHeaderView::ResizeToContents); //Disable changing row height.

    //Need to connect everytime we change the model.
    connect(tvBookmarks->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(tvBookmarksCurrentRowChanged(QModelIndex,QModelIndex)));
}

void BookmarksView::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
    tvBookmarks->setFocus();
}

QString BookmarksView::GetSelectedBookmarkName() const
{
    if (!filteredBookmarksModel)
        return QString();

    if (!tvBookmarks->currentIndex().isValid())
        return QString();

    int selRow = tvBookmarks->currentIndex().row();
    return filteredBookmarksModel->data(filteredBookmarksModel->index(selRow, dbm->bms.bidx.Name))
            .toString();
}

long long BookmarksView::GetSelectedBookmarkID() const
{
    if (!filteredBookmarksModel)
        return -1;

    if (!tvBookmarks->currentIndex().isValid())
        return -1;

    int selRow = tvBookmarks->currentIndex().row();
    long long selectedBId =
            filteredBookmarksModel->data(filteredBookmarksModel->index(selRow, dbm->bms.bidx.BID))
                                   .toLongLong();
    return selectedBId;
}

void BookmarksView::SelectBookmarkWithID(long long bookmarkId)
{
    if (!filteredBookmarksModel)
        return;

    QModelIndexList matches = filteredBookmarksModel->match(
                filteredBookmarksModel->index(0, dbm->bms.bidx.BID), Qt::DisplayRole,
                bookmarkId, 1, Qt::MatchExactly);

    if (matches.length() != 1)
        return; //Not found for some reason, e.g filtered out.

    tvBookmarks->setCurrentIndex(matches[0]);
    tvBookmarks->scrollTo(matches[0], QAbstractItemView::EnsureVisible);
}

bool BookmarksView::SetFilter(const BookmarkFilter& filter, bool forceReset)
{
    if (!filteredBookmarksModel)
        return false;
    return filteredBookmarksModel->SetFilter(filter, forceReset);
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

QItemSelectionModel* BookmarksView::selectionModel() const
{
    return tvBookmarks->selectionModel();
}

void BookmarksView::setCurrentIndex(const QModelIndex& index)
{
    tvBookmarks->setCurrentIndex(index);
}

QScrollBar*BookmarksView::horizontalScrollBar() const
{
    return tvBookmarks->horizontalScrollBar();
}

QScrollBar*BookmarksView::verticalScrollBar() const
{
    return tvBookmarks->verticalScrollBar();
}

void BookmarksView::RefreshView()
{
    //Currently this does nothing. Maybe the point is to remove it because things are automatic?
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

void BookmarksView::tvBookmarksCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    long long previousBID, currentBID;

    if (current.isValid())
    {
        int row = current.row();
        QModelIndex indexOfBID = filteredBookmarksModel->index(row, dbm->bms.bidx.BID);
        currentBID = filteredBookmarksModel->data(indexOfBID).toLongLong();
    }
    else
    {
        currentBID = -1;
    }

    if (previous.isValid())
    {
        int row = previous.row();
        QModelIndex indexOfBID = filteredBookmarksModel->index(row, dbm->bms.bidx.BID);
        previousBID = filteredBookmarksModel->data(indexOfBID).toLongLong();
    }
    else
    {
        previousBID = -1;
    }

    emit currentRowChanged(currentBID, previousBID);
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
