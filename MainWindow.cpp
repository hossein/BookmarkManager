#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "BookmarkEditDialog.h"

#include <QDebug>
#include <QDir>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMessageBox>
#include <QtGui/QResizeEvent>
#include <QtGui/QScrollBar>
#include <QtGui/QToolButton>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), dbm(this, &conf)
{
    ui->setupUi(this);

    // Set size and position
    int dWidth = QApplication::desktop()->availableGeometry().width();
    int dHeight = QApplication::desktop()->availableGeometry().height();
    resize(dWidth * 75/100, dHeight * 75/100);

    QRect geom = geometry();
    geom.moveCenter(QApplication::desktop()->availableGeometry().center());
    this->setGeometry(geom);

    // Set additional UI sizes
    QList<int> sizes;
    sizes << this->width() / 3 << this->width() * 2 / 3;
    ui->splitter->setSizes(sizes);

    // Add additional UI controls
    QMenu* importMenu = new QMenu("Import");
    importMenu->addAction(ui->action_importFirefoxBookmarks);

    QToolButton* btn = new QToolButton();
    btn->setText("Import/Export");
    btn->setMenu(importMenu);
    btn->setPopupMode(QToolButton::InstantPopup);
    //btn->setArrowType(Qt::LeftArrow);
    btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    ui->mainToolBar->addWidget(btn);

    // Load the application and logic
    LoadDatabaseAndUI();

    // Additional sub-parts initialization
    dbm.files.InitializeFilesDirectory();

    qApp->postEvent(this, new QResizeEvent(this->size(), this->size()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnNew_clicked()
{
    NewBookmark();
}

void MainWindow::on_btnEdit_clicked()
{
    EditSelectedBookmark();
}

void MainWindow::on_btnDelete_clicked()
{
    DeleteSelectedBookmark();
}

void MainWindow::on_tvBookmarks_activated(const QModelIndex &index)
{
    Q_UNUSED(index);
    EditSelectedBookmark();
}

void MainWindow::tvBookmarksCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);
    register bool valid = current.isValid();
    ui->btnEdit->setEnabled(valid);
    ui->btnDelete->setEnabled(valid);
}

void MainWindow::LoadDatabaseAndUI()
{
    bool success;

    QString databaseFilePath = QDir::currentPath() + "/" + conf.nominalDatabasetFileName;
    success = dbm.BackupOpenOrCreate(databaseFilePath);
    if (!success)
        return;

    RefreshUIDataDisplay(RA_Focus);
}

void MainWindow::RefreshUIDataDisplay(MainWindow::RefreshAction bookmarksAction, long long selectBID,
                                      MainWindow::RefreshAction tagsAction, long long selectTID)
{
    //TODO: What about sorting?
    //Calling this function after changes is needed, even for the bookmarks list.
    //  The model does NOT automatically update its view. Actually calliong `PopulateModels`
    //  DOES invalidate the selection, etc, but the tableView is not updated.

    //[SavingSelectedBookmarkAndTag]
    //For saving the currently selected bookmark, we save its row in the model only. Saving bookmark
    //  selection is just used when editing the bookmark, so likely its row in the model won't change.
    //BUT for saving the currently selected tag, we save its TId, because editing a bookmark may add
    //  another tags and change the currently selected tag's row.

    //[RestoringScrollPositionProceedsCustomSelection]
    //This is useful for tags, where we want to ensure the selected tag is visible even if new tags
    //  are added, while keeping the previous original scroll position if possible.

    int selectedTId = -1, selectedBRow = -1;
    int hTagsScrollPos = 0, hBScrollPos = 0, vBScrollPos = 0;

    //Save required things.
    if (bookmarksAction & RA_SaveSel) //[SavingSelectedBookmarkAndTag]
        if (ui->tvBookmarks->selectionModel()->selectedIndexes().size() > 0)
            selectedBRow = ui->tvBookmarks->selectionModel()->selectedIndexes()[0].row();
    if (tagsAction & RA_SaveSel) //[SavingSelectedBookmarkAndTag]
        selectedTId = GetSelectedTagID();
    if (bookmarksAction & RA_SaveScrollPos)
    {
        if (ui->tvBookmarks->horizontalScrollBar() != NULL)
            hBScrollPos = ui->tvBookmarks->horizontalScrollBar()->value();
        if (ui->tvBookmarks->verticalScrollBar() != NULL)
            vBScrollPos = ui->tvBookmarks->verticalScrollBar()->value();
    }
    if (tagsAction & RA_SaveScrollPos)
        if (ui->lwTags->verticalScrollBar() != NULL)
            hTagsScrollPos = ui->lwTags->verticalScrollBar()->value();

    //Real updating here
    dbm.PopulateModels();
    RefreshTagsDisplay();
    RefreshTVBookmarksModelView();

    //Pour out saved selections, scrolls, etc.
    if (bookmarksAction & RA_SaveSel)
        if (selectedBRow != -1)
            ui->tvBookmarks->setCurrentIndex(dbm.bms.model.index(selectedBRow, 0));
    if (tagsAction & RA_SaveSel)
        if (selectedTId != -1)
            SelectTagWithID(selectedTId);
    if (bookmarksAction & RA_SaveScrollPos)
    {
        if (ui->tvBookmarks->horizontalScrollBar() != NULL)
            ui->tvBookmarks->horizontalScrollBar()->setValue(hBScrollPos);
        if (ui->tvBookmarks->verticalScrollBar() != NULL)
            ui->tvBookmarks->verticalScrollBar()->setValue(vBScrollPos);
    }
    if (tagsAction & RA_SaveScrollPos)
        if (ui->lwTags->verticalScrollBar() != NULL)
            ui->lwTags->verticalScrollBar()->setValue(hTagsScrollPos);

    //[RestoringScrollPositionProceedsCustomSelection]
    if (bookmarksAction & RA_CustomSelect)
        if (selectBID != -1)
            SelectBookmarkWithID(selectBID);
    if (tagsAction & RA_CustomSelect)
        if (selectTID != -1)
            SelectTagWithID(selectTID);

    //Focusing comes last anyway
    if (bookmarksAction & RA_Focus)
        ui->tvBookmarks->setFocus();
    if (tagsAction & RA_Focus)
        ui->lwTags->setFocus();
}

void MainWindow::RefreshTVBookmarksModelView()
{
    ui->tvBookmarks->setModel(&dbm.bms.model);

    QHeaderView* hh = ui->tvBookmarks->horizontalHeader();

    BookmarkManager::BookmarkIndexes const& bidx = dbm.bms.bidx;
    if (hh->count() > 0) //This can happen on database errors.
    {
        hh->hideSection(bidx.BID);
        hh->hideSection(bidx.Desc);
        hh->hideSection(bidx.DefBFID);
        hh->hideSection(bidx.AddDate);

        hh->setResizeMode(bidx.Name, QHeaderView::Stretch);
        hh->resizeSection(bidx.URL, 200);
        //TODO: How to show tags? hh->resizeSection(dbm.bidx.Tags, 100);
        hh->resizeSection(bidx.Rating, 50);
    }

    QHeaderView* vh = ui->tvBookmarks->verticalHeader();
    vh->setResizeMode(QHeaderView::ResizeToContents); //Disable changing row height.

    //TODO: Connect everytime?
    connect(ui->tvBookmarks->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(tvBookmarksCurrentRowChanged(QModelIndex,QModelIndex)));
}

void MainWindow::NewBookmark()
{
    long long addedBId;
    BookmarkEditDialog* bmEditDialog = new BookmarkEditDialog(&dbm, -1, &addedBId, this);

    int result = bmEditDialog->exec();
    if (result != QDialog::Accepted)
        return;

    RefreshUIDataDisplay(RA_CustomSelectAndFocus, addedBId, RA_SaveSelAndScroll);
}

long long MainWindow::GetSelectedBookmarkID()
{
    if (!ui->tvBookmarks->currentIndex().isValid())
        return -1;

    int selRow = ui->tvBookmarks->currentIndex().row();
    int selectedBId =
            dbm.bms.model.data(dbm.bms.model.index(selRow, dbm.bms.bidx.BID)).toLongLong();
    return selectedBId;
}

void MainWindow::SelectBookmarkWithID(long long bookmarkId)
{
    QModelIndexList matches = dbm.bms.model.match(
                dbm.bms.model.index(0, dbm.bms.bidx.BID), Qt::DisplayRole,
                bookmarkId, 1, Qt::MatchExactly);

    if (matches.length() != 1)
        return; //Not found for some reason.

    ui->tvBookmarks->setCurrentIndex(matches[0]);
    ui->tvBookmarks->scrollTo(matches[0], QAbstractItemView::EnsureVisible);
}

void MainWindow::ViewSelectedBookmark()
{
    //TODO; and where is this called from?!
}

void MainWindow::EditSelectedBookmark()
{
    BookmarkEditDialog* bmEditDialog = new BookmarkEditDialog(&dbm, GetSelectedBookmarkID(), NULL, this);

    if (!bmEditDialog->canShow())
        return; //In case of errors a message is already shown.

    int result = bmEditDialog->exec();
    if (result != QDialog::Accepted)
        return;

    RefreshUIDataDisplay(RA_SaveSelAndScrollAndFocus, -1, RA_SaveSelAndScroll);
}

void MainWindow::DeleteSelectedBookmark()
{
    int selRow = ui->tvBookmarks->currentIndex().row();
    QString selectedBookmarkName =
            dbm.bms.model.data(dbm.bms.model.index(selRow, dbm.bms.bidx.Name)).toString();

    if (QMessageBox::Yes !=
        QMessageBox::question(this, "Delete Bookmark",
                              "Are you sure you want to delete the selected bookmark \""
                              + selectedBookmarkName + "\"?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
        return;

    int success = dbm.bms.DeleteBookmark(GetSelectedBookmarkID());
    if (!success)
        return;

    RefreshUIDataDisplay(RA_SaveScrollPosAndFocus, -1, RA_SaveSelAndScroll);
}

void MainWindow::RefreshTagsDisplay()
{
    //TODO: A bold "All Tags" item at top which get checked/unchecked when an item is
    //  selected/deselected.

    //Explains checkboxes with both QListWidget and QListView.
    //  http://www.qtcentre.org/threads/47119-checkbox-on-QListView

    QListWidgetItem* item;
    long long theTID;

    tagItems.clear();
    ui->lwTags->clear();

    const int TIDIdx = dbm.tags.tidx.TID;
    const int tagNameIdx = dbm.tags.tidx.TagName;
    int tagsCount = dbm.tags.model.rowCount();
    for (int i = 0; i < tagsCount; i++)
    {
        item = new QListWidgetItem(dbm.tags.model.data(dbm.tags.model.index(i, tagNameIdx)).toString());
        item->setCheckState(Qt::Unchecked);
        ui->lwTags->addItem(item);

        theTID = dbm.tags.model.data(dbm.tags.model.index(i, TIDIdx)).toLongLong();
        item->setData(Qt::UserRole + 0, theTID);
        tagItems[theTID] = item;
    }
}

long long MainWindow::GetSelectedTagID()
{
    if (ui->lwTags->selectedItems().size() == 0)
        return -1;
    return ui->lwTags->selectedItems()[0]->data(Qt::UserRole + 0).toLongLong();
}

void MainWindow::SelectTagWithID(long long tagId)
{
    if (!tagItems.contains(tagId))
        return;

    QListWidgetItem* item = tagItems[tagId];

    ui->lwTags->selectedItems().clear();
    ui->lwTags->selectedItems().append(item);
    ui->lwTags->scrollToItem(item, QAbstractItemView::EnsureVisible);
}
