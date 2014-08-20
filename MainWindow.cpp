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
    QMainWindow(parent), ui(new Ui::MainWindow), conf(), dbm(this, &conf),
    filteredBookmarksModel(&dbm, this, &conf, this)
{
    ui->setupUi(this);
    //TODO: Tag something with `c++` and upon saving it's changed!

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

void MainWindow::lwTagsItemChanged(QListWidgetItem* item)
{
    //Disconnect this signal-slot connection to make sure ItemChanged
    //  signals for lwTags are not fired while updating the tag item's checkState.
    //  We connect it at function's end.
    //static int i = 0;
    //qDebug() << "CHANGE" << i++;
    //Strange: This doesn't work:
    //disconnect(ui->lwTags, SIGNAL(itemChanged(QListWidgetItem*)));
    ui->lwTags->disconnect(SIGNAL(itemChanged(QListWidgetItem*)));

    long long theTID = item->data(Qt::UserRole + 0).toLongLong();

    if (theTID == -1)
    {
        //The "All Tags" item was checked/unchecked. Check/Uncheck all tags.
        Qt::CheckState checkState = item->checkState();
        foreach (QListWidgetItem* item, tagItems.values())
            item->setCheckState(checkState);
    }
    else
    {
        //A tag item was checked/unchecked. Set the check state of the "All Tags" item.
        TagCheckStateResult tagsCheckState = areAllTagsChecked();
        if (tagsCheckState == TCSR_NoneChecked)
            ui->lwTags->item(0)->setCheckState(Qt::Unchecked);
        else if (tagsCheckState == TCSR_SomeChecked)
            ui->lwTags->item(0)->setCheckState(Qt::PartiallyChecked);
        else if (tagsCheckState == TCSR_AllChecked)
            ui->lwTags->item(0)->setCheckState(Qt::Checked);
    }

    //Connect the signal we disconnected.
    connect(ui->lwTags, SIGNAL(itemChanged(QListWidgetItem*)),
            this, SLOT(lwTagsItemChanged(QListWidgetItem*)));

    //TODO: If you check C++ tag and the second bookmark is the only filtered bookmark, double-clicking
    //  it opens the Edit dialog for the FIRST bookmark! Is it related to the following TODO? That we
    //  have to use RefreshUIDataDisplay for it?

    //Now filter the bookmarks according to those tags.
    //Note: Instead of just `RefreshTVBookmarksModelView()` we use the full Refresh below.
    //  It's more standard for this task; we want to save selection, and also update status labels.
    //  We don't populate the data again though.
    RefreshUIDataDisplay(false, RA_SaveSelAndFocus, -1,
                         (RefreshAction)(RA_SaveSelAndScroll | RA_NoRefreshView));
}

void MainWindow::LoadDatabaseAndUI()
{
    bool success;

    QString databaseFilePath = QDir::currentPath() + "/" + conf.nominalDatabasetFileName;
    success = dbm.BackupOpenOrCreate(databaseFilePath);
    if (!success)
        return;

    RefreshUIDataDisplay(true, RA_Focus);
}

void MainWindow::RefreshUIDataDisplay(bool rePopulateModels,
                                      MainWindow::RefreshAction bookmarksAction, long long selectBID,
                                      MainWindow::RefreshAction tagsAction, long long selectTID,
                                      const QList<long long>& newTIDsToCheck)
{
    //TODO: What about sorting?

    //TODO: If some or all tags are checked, we need to preserve their check state.
    //  ALSO: If a new tag is added in this situation, it needs to become Checked so that it's
    //  visible in the tags list.

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

    //Disconnect this signal-slot connection to make sure ItemChanged
    //  signals for lwTags are not fired while updating the tags view.
    //  We connect it at function's end.
    ui->lwTags->disconnect(SIGNAL(itemChanged(QListWidgetItem*)));

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
    //ONLY care about check states if there were already tag FILTERations.
    //TODO: Can cache the result of `areAllTagsChecked` in "All Items" check box.
    bool mustCareAboutCheckStates = (TCSR_SomeChecked == areAllTagsChecked());
    QList<long long> checkedTIDs;
    if ((tagsAction & RA_SaveCheckState) && mustCareAboutCheckStates)
        checkedTIDs  = GetCheckedTIDs();

    //Real updating here
    if (rePopulateModels)
        dbm.PopulateModels();

    if (!(tagsAction & RA_NoRefreshView))
        RefreshTagsDisplay();

    //Now make sure those we want to check are checked.
    //IMPORTANT: Must do it early, as the next function is immediately `RefreshTVBookmarksModelView`
    //  which RELIES on the checks.
    //Note: If `!mustCareAboutCheckStates` then the `checkedTIDs` list is already empty,
    //  however the user supplied `newTIDsToCheck` contains items so we do need the
    //  `mustCareAboutCheckStates` at the beginning to prevent checking additional items.
    if ((tagsAction & RA_SaveCheckState) && mustCareAboutCheckStates)
    {
        //New TIDs are already added to `tagItems` as the tags are refreshed above.
        //  So it's safe that the following function references `tagItems[newCheckedTID]`.
        //  Also we are checking additional items ONLY IF RA_SaveCheckState is set.
        RestoreCheckedTIDs(checkedTIDs, newTIDsToCheck);
    }

    if (!(bookmarksAction & RA_NoRefreshView))
        RefreshTVBookmarksModelView();

    //Refresh status labels.
    RefreshStatusLabels();

    //Pour out saved selections, scrolls, etc.
    if (bookmarksAction & RA_SaveSel)
        if (selectedBRow != -1)
            ui->tvBookmarks->setCurrentIndex(filteredBookmarksModel.index(selectedBRow, 0));
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

    //Connect the signal we disconnected.
    connect(ui->lwTags, SIGNAL(itemChanged(QListWidgetItem*)),
            this, SLOT(lwTagsItemChanged(QListWidgetItem*)));
}

void MainWindow::RefreshStatusLabels()
{
    //NOTE: Again, we can cache this.
    bool allTagsOrNoneOfTheTags = ui->lwTags->item(0)->checkState() == Qt::Checked
                               || ui->lwTags->item(0)->checkState() == Qt::Unchecked;

    if (allTagsOrNoneOfTheTags)
    {
        ui->lblFilter->setText("Showing All Bookmarks");
        ui->lblBMCount->setText(QString("%1 Bookmarks").arg(dbm.bms.model.rowCount()));
    }
    else
    {
        QString checkedTagsNameList;
        foreach (QListWidgetItem* tagItem, tagItems) //Values
            if (tagItem->checkState() == Qt::Checked)
                checkedTagsNameList += tagItem->text() + ", ";
        //We are sure there was at least one tag selected.
        checkedTagsNameList.chop(2);

        ui->lblFilter->setText("Filtering By Tags: " + checkedTagsNameList);
        ui->lblBMCount->setText(QString("%1/%2 Bookmarks")
                                .arg(filteredBookmarksModel.rowCount())
                                .arg(dbm.bms.model.rowCount()));
    }
}

void MainWindow::RefreshTVBookmarksModelView()
{
    //First we check if "All Items" is checked or not. If it's fully checked or fully unchecked,
    //  we don't filter by tags.
    bool allTagsOrNoneOfTheTags = ui->lwTags->item(0)->checkState() == Qt::Checked
                               || ui->lwTags->item(0)->checkState() == Qt::Unchecked;

    //TODO: Neede everytime?
    filteredBookmarksModel.setSourceModel(&dbm.bms.model);
    if (allTagsOrNoneOfTheTags)
    {
        filteredBookmarksModel.ClearFilters();
    }
    else
    {
        //"All Items" does not appear in `tagItems`. Even if it was, it was excluded by the Qt::Checked
        //  condition, as we have already checked `allTagsOrNoneOfTheTags`.
        QSet<long long> tagIDs;
        for (auto it = tagItems.constBegin(); it != tagItems.constEnd(); ++it)
            if (it.value()->checkState() == Qt::Checked)
                tagIDs.insert(it.key());

        filteredBookmarksModel.FilterSpecificTagIDs(tagIDs);
    }

    ui->tvBookmarks->setModel(&filteredBookmarksModel);

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
    BookmarkEditDialog::OutParams outParams;
    BookmarkEditDialog* bmEditDialog = new BookmarkEditDialog(&dbm, -1, &outParams, this);

    int result = bmEditDialog->exec();
    if (result != QDialog::Accepted)
        return;

    RefreshUIDataDisplay(true, RA_CustomSelectAndFocus, outParams.addedBId, RA_SaveSelAndScrollAndCheck,
                         -1, outParams.associatedTIDs);
}

long long MainWindow::GetSelectedBookmarkID()
{
    if (!ui->tvBookmarks->currentIndex().isValid())
        return -1;

    int selRow = ui->tvBookmarks->currentIndex().row();
    int selectedBId =
            filteredBookmarksModel.data(filteredBookmarksModel.index(selRow, dbm.bms.bidx.BID))
                                  .toLongLong();
    return selectedBId;
}

void MainWindow::SelectBookmarkWithID(long long bookmarkId)
{
    QModelIndexList matches = filteredBookmarksModel.match(
                filteredBookmarksModel.index(0, dbm.bms.bidx.BID), Qt::DisplayRole,
                bookmarkId, 1, Qt::MatchExactly);

    if (matches.length() != 1)
        return; //Not found for some reason, e.g filtered out.

    ui->tvBookmarks->setCurrentIndex(matches[0]);
    ui->tvBookmarks->scrollTo(matches[0], QAbstractItemView::EnsureVisible);
}

void MainWindow::ViewSelectedBookmark()
{
    //TODO; and where is this called from?!
}

void MainWindow::EditSelectedBookmark()
{
    BookmarkEditDialog::OutParams outParams;
    BookmarkEditDialog* bmEditDialog = new BookmarkEditDialog(&dbm, GetSelectedBookmarkID(), &outParams,
                                                              this);

    if (!bmEditDialog->canShow())
        return; //In case of errors a message is already shown.

    int result = bmEditDialog->exec();
    if (result != QDialog::Accepted)
        return;

    RefreshUIDataDisplay(true, RA_SaveSelAndScrollAndFocus, -1, RA_SaveSelAndScrollAndCheck,
                         -1, outParams.associatedTIDs);
}

void MainWindow::DeleteSelectedBookmark()
{
    int selRow = ui->tvBookmarks->currentIndex().row();
    QString selectedBookmarkName =
            filteredBookmarksModel.data(filteredBookmarksModel.index(selRow, dbm.bms.bidx.Name))
                                  .toString();

    if (QMessageBox::Yes !=
        QMessageBox::question(this, "Delete Bookmark",
                              "Are you sure you want to delete the selected bookmark \""
                              + selectedBookmarkName + "\"?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
        return;

    int success = dbm.bms.DeleteBookmark(GetSelectedBookmarkID());
    if (!success)
        return;

    RefreshUIDataDisplay(true, RA_SaveScrollPosAndFocus, -1, RA_SaveSelAndScrollAndCheck);
}

void MainWindow::RefreshTagsDisplay()
{
    //Explains checkboxes with both QListWidget and QListView.
    //  http://www.qtcentre.org/threads/47119-checkbox-on-QListView

    QListWidgetItem* item;
    long long theTID;

    tagItems.clear();
    ui->lwTags->clear();

    //Add the bold "All Tags" item at top which get checked/unchecked when an item is
    //  selected/deselected.
    //We don't need to keep the "All Tags" item separate.
    //  We simply don't put it in `tagItems` to not require skipping the first item or something.
    item = new QListWidgetItem("All Tags");
    QFont boldFont(this->font());
    boldFont.setBold(true);
    item->setFont(boldFont);
    item->setCheckState(Qt::Unchecked);
    item->setData(Qt::UserRole + 0, -1);
    ui->lwTags->addItem(item);

    //Add the rest of the tags.
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

MainWindow::TagCheckStateResult MainWindow::areAllTagsChecked()
{
    bool seenChecked = false;
    bool seenUnchecked = false;
    foreach (QListWidgetItem* item, tagItems.values())
    {
        if (seenChecked && seenUnchecked)
            break;

        if (item->checkState() == Qt::Checked)
            seenChecked = true;
        else if (item->checkState() == Qt::Unchecked)
            seenUnchecked = true;
    }

    if (seenChecked && seenUnchecked)
        return TCSR_SomeChecked;
    else if (seenChecked) // && !seenUnchecked
        return TCSR_AllChecked;
    else if (seenUnchecked) // && !seenChecked
        return TCSR_NoneChecked;
    else //Empty tag items.
        return TCSR_NoneChecked;
}

QList<long long> MainWindow::GetCheckedTIDs()
{
    QList<long long> checkedTIDs;
    for (auto it = tagItems.constBegin(); it != tagItems.constEnd(); ++it)
        if (it.value()->checkState() == Qt::Checked) //"All Items" does not appear in `tagItems`.
            checkedTIDs.append(it.key());
    return checkedTIDs;
}

void MainWindow::RestoreCheckedTIDs(const QList<long long>& checkedTIDs,
                                    const QList<long long>& newTIDsToCheck)
{
    foreach (long long checkTID, checkedTIDs)
        tagItems[checkTID]->setCheckState(Qt::Checked);

    foreach (long long checkTID, newTIDsToCheck)
        tagItems[checkTID]->setCheckState(Qt::Checked);

    //NOTE: And yet again we have to cache this.
    //  `RefreshTVBookmarksModelView` RELIES on it.
    TagCheckStateResult tagsCheckState = areAllTagsChecked();
    if (tagsCheckState == TCSR_NoneChecked)
        ui->lwTags->item(0)->setCheckState(Qt::Unchecked);
    else if (tagsCheckState == TCSR_SomeChecked)
        ui->lwTags->item(0)->setCheckState(Qt::PartiallyChecked);
    else if (tagsCheckState == TCSR_AllChecked)
        ui->lwTags->item(0)->setCheckState(Qt::Checked);
}
