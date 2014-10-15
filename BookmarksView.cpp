#include "BookmarksView.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableView>

#include "BookmarksFilteredByTagsSortProxyModel.h"

BookmarksView::BookmarksView(QWidget *parent)
    : QWidget(parent)
    , dialogParent(parent), tvBookmarks(NULL), dbm(NULL), conf(NULL), filteredBookmarksModel(NULL)
{
}

void BookmarksView::Initialize(DatabaseManager* dbm, Config* conf, ListMode listMode)
{
    //Class members
    this->dbm = dbm;
    this->conf = conf;
    this->m_listMode = listMode;

    filteredBookmarksModel = new BookmarksFilteredByTagsSortProxyModel(dbm, dialogParent, conf, this);

    //UI
    tvBookmarks = new QTableView(this);
    tvBookmarks->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tvBookmarks->setSelectionMode(QAbstractItemView::SingleSelection);
    tvBookmarks->setSelectionBehavior(QAbstractItemView::SelectRows);
    tvBookmarks->setWordWrap(false);

    QHeaderView* hh = tvBookmarks->horizontalHeader();
    QHeaderView* vh = tvBookmarks->verticalHeader();
    hh->setVisible(listMode != LM_NameDisplayOnly);
    hh->setHighlightSections(false);
    vh->setVisible(false);
    vh->setHighlightSections(false);

    QHBoxLayout* myLayout = new QHBoxLayout();
    myLayout->addWidget(tvBookmarks);
    myLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(myLayout);

    //First time connections.
    connect(tvBookmarks, SIGNAL(activated(QModelIndex)), this, SLOT(tvBookmarksActivated(QModelIndex)));
    connect(hh, SIGNAL(sectionPressed(int)), this, SLOT(tvBookmarksHeaderPressed(int)));
    connect(hh, SIGNAL(sectionClicked(int)), this, SLOT(tvBookmarksHeaderClicked(int)));
}

QString BookmarksView::GetSelectedBookmarkName() const
{
    if (!tvBookmarks->currentIndex().isValid())
        return QString();

    int selRow = tvBookmarks->currentIndex().row();
    return filteredBookmarksModel->data(filteredBookmarksModel->index(selRow, dbm->bms.bidx.Name))
            .toString();
}

long long BookmarksView::GetSelectedBookmarkID() const
{
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
    QModelIndexList matches = filteredBookmarksModel->match(
                filteredBookmarksModel->index(0, dbm->bms.bidx.BID), Qt::DisplayRole,
                bookmarkId, 1, Qt::MatchExactly);

    if (matches.length() != 1)
        return; //Not found for some reason, e.g filtered out.

    tvBookmarks->setCurrentIndex(matches[0]);
    tvBookmarks->scrollTo(matches[0], QAbstractItemView::EnsureVisible);
}

void BookmarksView::ClearFilters()
{
    filteredBookmarksModel->ClearFilters();
}

bool BookmarksView::FilterSpecificBookmarkIDs(const QList<long long>& BIDs)
{
    return filteredBookmarksModel->FilterSpecificBookmarkIDs(BIDs);
}

bool BookmarksView::FilterSpecificTagIDs(const QSet<long long>& tagIDs)
{
    return filteredBookmarksModel->FilterSpecificTagIDs(tagIDs);
}

int BookmarksView::GetTotalBookmarksCount() const
{
    return filteredBookmarksModel->sourceModel()->rowCount();
}

int BookmarksView::GetDisplayedBookmarksCount() const
{
    return filteredBookmarksModel->rowCount();
}

void BookmarksView::setModel(QAbstractItemModel* model)
{
    filteredBookmarksModel->setSourceModel(model);
    tvBookmarks->setModel(filteredBookmarksModel);
}

QItemSelectionModel*BookmarksView::selectionModel() const
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

void BookmarksView::ResetHeadersAndSort()
{
    QHeaderView* hh = tvBookmarks->horizontalHeader();
    //TODO: Sorts must be here?
    Qt::SortOrder sortOrder = hh->sortIndicatorOrder();
    int sortColumn = hh->sortIndicatorSection();

    //TODO: Seems this can be done only once, like the signal connection.
    BookmarkManager::BookmarkIndexes const& bidx = dbm->bms.bidx;
    if (hh->count() > 0) //This can happen on database errors.
    {
        hh->hideSection(bidx.BID);
        hh->hideSection(bidx.Desc);
        hh->hideSection(bidx.DefBFID);
        hh->hideSection(bidx.AddDate);

        if (m_listMode <= LM_LimitedDisplayOnly)
            hh->hideSection(bidx.URL);
        if (m_listMode <= LM_NameDisplayOnly)
            hh->hideSection(bidx.Rating);

        hh->setResizeMode(bidx.Name, QHeaderView::Stretch);

        hh->resizeSection(bidx.URL, 200);
        //TODO: How to show tags? hh->resizeSection(dbm.bidx.Tags, 100);
        hh->resizeSection(bidx.Rating, 50);
    }

    //ui->tvBookmarks->sortByColumn(sortColumn);
    filteredBookmarksModel->sort(sortColumn, sortOrder);
    //hh->setSortIndicator(sortColumn, sortOrder);

    QHeaderView* vh = tvBookmarks->verticalHeader();
    vh->setResizeMode(QHeaderView::ResizeToContents); //Disable changing row height.

    //Need to connect everytime we change the model.
    connect(tvBookmarks->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(tvBookmarksCurrentRowChanged(QModelIndex,QModelIndex)));
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

    QHeaderView* hh = tvBookmarks->horizontalHeader();
    hh->setSortIndicatorShown(sortNextLogicalIndex != -1);
    hh->setSortIndicator(sortNextLogicalIndex, sortNextOrder);

    if (sortNextLogicalIndex == -1)
        filteredBookmarksModel->sort(dbm->bms.bidx.BID, Qt::AscendingOrder);
    else
        filteredBookmarksModel->sort(logicalIndex, sortNextOrder);
}
