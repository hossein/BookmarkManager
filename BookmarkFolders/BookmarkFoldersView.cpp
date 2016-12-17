#include "BookmarkFoldersView.h"

#include "BookmarkFolderEditDialog.h"
#include "BookmarkFoldersTreeWidget.h"
#include "Database/DatabaseManager.h"

#include <QQueue>

#include <QAction>
#include <QFocusEvent>
#include <QHeaderView>
#include <QToolBar>
#include <QVBoxLayout>

BookmarkFoldersView::BookmarkFoldersView(QWidget *parent)
    : QWidget(parent), dbm(NULL), m_onceNoEmitChangeFOID(false), m_lastEmittedChangeFOID(-1)
{
    //Initialize this here to protect from some crashes
    twFolders = new BookmarkFoldersTreeWidget(this);
    twFolders->setAcceptDrops(true); // < v Either one was enough
    twFolders->setDragDropMode(QAbstractItemView::DropOnly);
    twFolders->setDropIndicatorShown(false);
    twFolders->setDragDropOverwriteMode(true);
    twFolders->setEditTriggers(QAbstractItemView::NoEditTriggers);
    twFolders->setSelectionMode(QAbstractItemView::SingleSelection);
    twFolders->setSelectionBehavior(QAbstractItemView::SelectRows);
    twFolders->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    twFolders->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    twFolders->setWordWrap(false);
    twFolders->header()->close();
}

BookmarkFoldersView::~BookmarkFoldersView()
{

}

void BookmarkFoldersView::Initialize(DatabaseManager* dbm)
{
    this->dbm = dbm;
    twFolders->Initialize(dbm->conf);

    //Create the UI
    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(0);
    vLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(vLayout);

    QToolBar* fToolbar = new QToolBar("Folders Toolbar", this);
    m_newAction = fToolbar->addAction(QIcon(":/res/folder-new.png"), "New Sub-Folder", this, SLOT(btnNewFolderClicked()));
    m_editAction = fToolbar->addAction(QIcon(":/res/folder-edit.png"), "Edit Folder", this, SLOT(btnEditFolderClicked()));
    fToolbar->addSeparator();
    m_deleteAction = fToolbar->addAction(QIcon(":/res/folder-delete.png"), "Delete Folder", this, SLOT(btnDeleteFolderClicked()));
    m_deleteAction->setShortcut(QKeySequence("Delete"));

    vLayout->addWidget(fToolbar);
    vLayout->addWidget(twFolders, 1);

    //Add Folder Items
    AddItems(false);

    //Connections
    connect(twFolders,   SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(twFoldersCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(twFolders, SIGNAL(RequestMoveBookmarksToFolder(QList<long long>,long long)),
            this,      SIGNAL(RequestMoveBookmarksToFolder(QList<long long>,long long)));

    //Select the '0, Unsorted' folder and expand all items.
    twFolders->setCurrentItem(m_itemForFOID[0]); //Must be done AFTER CONNECTION to disable delete button.
    twFolders->expandAll();
}

long long BookmarkFoldersView::GetCurrentFOID()
{
    if (twFolders->currentItem() == NULL)
        return 0; //Not possible; Just in case, e.g for uninitialized state.
    return twFolders->currentItem()->data(0, Qt::UserRole+0).toLongLong();
}

void BookmarkFoldersView::SetCurrentFOIDSilently(long long FOID)
{
    if (FOID == GetCurrentFOID())
        return; //Otherwise setting `m_onceNoEmitChangeFOID = true` will cause problems.
    m_onceNoEmitChangeFOID = true; //Prevent emiting CurrentFolderChanged.
    twFolders->setCurrentItem(m_itemForFOID[FOID]);
}

void BookmarkFoldersView::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
    if (event->reason() != Qt::MouseFocusReason) //Don't focus the tw if user clicked on toolbar.
        twFolders->setFocus();
}

void BookmarkFoldersView::AddItems(bool rememberExpands)
{
    if (rememberExpands)
        RememberExpands();

    m_itemForFOID.clear();
    twFolders->clear();

    QList<long long> foldersOrder;
    QList<long long> bookmarkFolderKeys = dbm->bfs.bookmarkFolders.keys();

    //We can't assume all parents come before their children, so we do it using a queue to make sure
    //  all parents can be created before their children.in a nested for loop.
    QQueue<long long> foldersQueue;
    foldersQueue.enqueue(-1); //The fake '-1, All Bookmarks' folder. It's also the parent of the
                              //'0, Unsorted', which is itself the parent of all others.
    while (!foldersQueue.isEmpty())
    {
        long long ParentFOID = foldersQueue.dequeue();
        foldersOrder.append(ParentFOID);

        //We use a map of Name->FOID to sort the children by its keys, i.e folder names.
        QMap<QString, long long> childrenMap;
        foreach (long long FOID, bookmarkFolderKeys)
            if (dbm->bfs.bookmarkFolders[FOID].ParentFOID == ParentFOID &&
                dbm->bfs.bookmarkFolders[FOID].FOID != -1) //Parent of '-1, All Bookmarks' folder is also -1.
                childrenMap.insert(dbm->bfs.bookmarkFolders[FOID].Name, FOID);

        //Now children are sorted by their names
        foreach (long long FOID, childrenMap)
            foldersQueue.enqueue(FOID);
    }

    //Now we have an alphabetically sorted tree of folders, and we are sure each parent is added
    //  before all its children!
    m_itemForFOID.clear();
    foreach (long long FOID, foldersOrder)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << dbm->bfs.bookmarkFolders[FOID].Name);
        item->setToolTip(0, dbm->bfs.bookmarkFolders[FOID].Desc);
        item->setData(0, Qt::UserRole+0, FOID);
        m_itemForFOID[FOID] = item;

        //Show '0, Unsorted' and '-1, All Bookmarks' folders in bold font.
        if (FOID <= 0)
        {
            QFont boldFont = item->font(0);
            boldFont.setBold(true);
            item->setFont(0, boldFont);
        }

        if (FOID <= 0) //'0, Unsorted' and '-1, All Bookmarks' folders are always top-level items.
            twFolders->addTopLevelItem(item);
        else if (dbm->bfs.bookmarkFolders[FOID].ParentFOID == 0)
            //SPECIAL CASE: Although technically every folder is the child of the '0, Unsorted'
            //  folder, we VISUALLY show its immediate children as top-level folders.
            twFolders->addTopLevelItem(item);
        else
            m_itemForFOID[dbm->bfs.bookmarkFolders[FOID].ParentFOID]->addChild(item);
    }

    if (rememberExpands)
        RestoreExpands();
}

void BookmarkFoldersView::RememberExpands()
{
    m_expandedState.clear();
    QList<QTreeWidgetItem*> items = twFolders->findItems("", Qt::MatchContains | Qt::MatchRecursive);
    foreach (QTreeWidgetItem* item, items)
        if (item->childCount() > 0)
            m_expandedState[item->data(0, Qt::UserRole+0).toLongLong()] = item->isExpanded();
}

void BookmarkFoldersView::RestoreExpands()
{
    foreach (long long FOID, m_expandedState.keys())
        m_itemForFOID[FOID]->setExpanded(m_expandedState[FOID]);
}

void BookmarkFoldersView::twFoldersCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    Q_UNUSED(previous);
    //Don't enable for '0, Unsorted' and '-1, All Bookmarks' folders.
    //`current` becomes NULL when clearing items.
    bool enabled = (current != NULL && current->data(0, Qt::UserRole+0).toLongLong() > 0);
    m_editAction->setEnabled(enabled);
    m_deleteAction->setEnabled(enabled);

    if (current != NULL)
    {
        long long currentFOID = current->data(0, Qt::UserRole+0).toLongLong();
        if (m_lastEmittedChangeFOID != currentFOID)
        {
            if (!m_onceNoEmitChangeFOID)
            {
                emit CurrentFolderChanged(currentFOID);
                m_lastEmittedChangeFOID = currentFOID;
            }
            m_onceNoEmitChangeFOID = false;
        }
    }
}

void BookmarkFoldersView::btnNewFolderClicked()
{
    long long parentFOID = twFolders->currentItem()->data(0, Qt::UserRole+0).toLongLong();

    BookmarkFolderEditDialog::OutParams outParams;
    BookmarkFolderEditDialog foEditDialog(dbm, -1, parentFOID, &outParams, this);

    if (!foEditDialog.canShow())
        return; //In case of errors a message is already shown.

    int result = foEditDialog.exec();
    if (result != QDialog::Accepted)
        return;

    //Remember previous expands and add items. Select the added item and make sure its parent is
    //  expanded, if it previously wasn't (e.g if it didn't have children).
    AddItems(true);
    m_itemForFOID[parentFOID]->setExpanded(true);
    twFolders->setCurrentItem(m_itemForFOID[outParams.addedFOID]);
}

void BookmarkFoldersView::btnEditFolderClicked()
{
    long long currentFOID = twFolders->currentItem()->data(0, Qt::UserRole+0).toLongLong();
    if (currentFOID == 0)
        return; //Not possible. Just in case.

    BookmarkFolderEditDialog foEditDialog(dbm, currentFOID, -1, NULL, this);

    if (!foEditDialog.canShow())
        return; //In case of errors a message is already shown.

    int result = foEditDialog.exec();
    if (result != QDialog::Accepted)
        return;

    //Remember previous expands and add items. Select the current item.
    AddItems(true);
    twFolders->setCurrentItem(m_itemForFOID[currentFOID]);
}

void BookmarkFoldersView::btnDeleteFolderClicked()
{
    long long currentFOID = twFolders->currentItem()->data(0, Qt::UserRole+0).toLongLong();
    if (currentFOID == 0)
        return; //Not possible. Just in case.

    QString selectedFolderName = dbm->bfs.bookmarkFolders[currentFOID].Name;
    long long selectedFolderParent = dbm->bfs.bookmarkFolders[currentFOID].ParentFOID;

    if (QMessageBox::Yes !=
        QMessageBox::question(this, "Delete folder",
                              "Are you sure you want to delete the folder \"" + selectedFolderName + "\"?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
        return;

    //Check to make sure the Folder does not have any sub-folders or bookmarks.
    QString cantDeleteError =
            "Could not delete the selected folder because it contains %1.\n"
            "Before deleting a folder move or delete all its sub-folders and bookmarks.";
    if (!dbm->bfs.GetChildrenIDs(currentFOID).isEmpty())
    {
        QMessageBox::warning(this, "Cannot delete folder", cantDeleteError.arg("sub-folders"));
        return;
    }
    int bookmarksCount = -1;
    if (!dbm->bms.CountBookmarksInFolder(bookmarksCount, currentFOID))
    {
        //Message already shown
        return;
    }
    if (bookmarksCount != 0)
    {
        QMessageBox::warning(this, "Cannot delete folder", cantDeleteError.arg("bookmarks"));
        return;
    }

    //Remove the folder.
    bool success = dbm->bfs.RemoveBookmarkFolder(currentFOID);
    if (!success)
        return;

    //Remember previous expands and add items. Select the parent of the deleted item.
    AddItems(true);
    twFolders->setCurrentItem(m_itemForFOID[selectedFolderParent]);
}
