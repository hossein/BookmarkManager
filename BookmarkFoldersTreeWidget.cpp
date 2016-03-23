#include "BookmarkFoldersTreeWidget.h"

#include "Config.h"

#include <QList>
#include <QMimeData>
#include <QDropEvent>
#include <QDragMoveEvent>

BookmarkFoldersTreeWidget::BookmarkFoldersTreeWidget(QWidget* parent)
    : QTreeWidget(parent), conf(NULL)
{

}

BookmarkFoldersTreeWidget::~BookmarkFoldersTreeWidget()
{

}

void BookmarkFoldersTreeWidget::Initialize(Config* conf)
{
    this->conf = conf;
}

bool BookmarkFoldersTreeWidget::dropResultsInValidIndex(const QPoint& pos)
{
    QTreeWidgetItem* item = itemAt(pos);
    if (item == NULL || !indexFromItem(item).isValid())
        return false;
    //Copied from QAbstractItemViewPrivate::position
    return visualRect(indexFromItem(item)).adjusted(-1, -1, 1, 1).contains(pos, false);
}

void BookmarkFoldersTreeWidget::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeWidget::dragMoveEvent(event);
    if (!event->isAccepted())
        return;

    if (event->mouseButtons() != Qt::LeftButton)
    {
        //Allow only left mouse button alone.
        //We wish we could cancel this completely.
        event->ignore();
        return;
    }

    if (dropResultsInValidIndex(event->pos()))
        event->accept();
    else
        event->ignore(); //Show 'forbidden' cursor.
}

void BookmarkFoldersTreeWidget::dropEvent(QDropEvent* event)
{
    if (event->mouseButtons() != Qt::LeftButton)
    {
        //Allow only left mouse button alone.
        event->ignore();
        return;
    }

    //Not Needed:
    //  if (!event->mimeData()->hasFormat(conf->mimeTypeBookmarks))
    //  {
    //      event->ignore();
    //      return;
    //  }
    //Instead we reimplemented `mimeTypes`

    //Not Needed:
    //  //We only support this action; But make sure it doesn't remove items from the bookmarks
    //  //  TableView and model.
    //  event->setDropAction(Qt::MoveAction);
    //Instead we reimplemented `supportedDropActions`.

    //From the docs:
    //  "The QMimeData and QDrag objects created by the source widget should not be deleted -
    //   they will be destroyed by Qt."
    //delete event->mimeData();

    //Do Not Do This Here:
    //  //Instead of QTreeWidget::dropEvent(event), we use these.
    //  //These are from QAbstractItemView::dropAction:
    //  stopAutoScroll();
    //  setState(QAbstractItemView::NoState);
    //  viewport()->update();
    //Instead we reimplemented `dropMimeData` and do call `QTreeWidget::dropEvent`.
    return QTreeWidget::dropEvent(event);
}

bool BookmarkFoldersTreeWidget::dropMimeData(QTreeWidgetItem* parent, int index, const QMimeData* data, Qt::DropAction action)
{
    Q_UNUSED(index);
    if (parent == NULL)
        return false;
    if (action != Qt::MoveAction)
        return false;
    //This must match BookmarkFoldersTreeWidget::supportedDropActions.
    if (!data->hasFormat(conf->mimeTypeBookmarks))
        return false;

    QModelIndex modelIndex = indexFromItem(parent);
    if (!modelIndex.isValid()) //Not probable
        return false;

    long long FOID = modelIndex.data(Qt::UserRole).toLongLong();
    QList<long long> BIDs;
    foreach (const QByteArray& bnum, data->data(conf->mimeTypeBookmarks).split(','))
        BIDs.append(bnum.toLongLong());

    emit RequestMoveBookmarksToFolder(BIDs, FOID);

    //Not: return QTreeWidget::dropMimeData(parent, index, data, action);
    //Always return true, we handled the data anyway, even if we encountered errors.
    return true;
}

QStringList BookmarkFoldersTreeWidget::mimeTypes() const
{
    if (conf != NULL)
        return QStringList() << conf->mimeTypeBookmarks;
    return QStringList();
}

Qt::DropActions BookmarkFoldersTreeWidget::supportedDropActions() const
{
    //QTreeWidget::supportedDropActions() supports Copy and Move.
    //This must match BookmarksSortFilterProxyModel::supportedDragActions and
    //  BookmarkFoldersTreeWidget::dropMimeData.
    return Qt::MoveAction;
}
