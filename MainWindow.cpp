#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "BookmarkFilter.h"
#include "BookmarkEditDialog.h"
#include "BookmarkViewDialog.h"
#include "BookmarksBusinessLogic.h"

#include "BookmarkImporter.h"
#include "ImportedBookmarksPreviewDialog.h"
#include "BookmarkImporters/FirefoxBookmarkJSONFileParser.h"

#include <QDebug>
#include <QDir>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QScrollBar>
#include <QToolButton>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), conf(), dbm(this, &conf), m_shouldExit(false),
    m_allTagsChecked(TCSR_NoneChecked)
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
    QList<int> hsizes, vsizes;
    hsizes << this->width() / 3 << this->width() * 2 / 3;
    ui->splitter->setSizes(hsizes);
    vsizes << this->height() * 2 / 3 << this->height() / 3;
    ui->splitterFT->setSizes(vsizes);

    // Add additional UI controls
    QMenu* importMenu = new QMenu("Import");
    importMenu->addAction(ui->
                          action_importFirefoxBookmarks);
    importMenu->addAction(ui->actionImportFirefoxBookmarksJSONfile);
    importMenu->addAction(ui->actionGetMHT);

    QToolButton* btn = new QToolButton();
    btn->setText("Import/Export");
    btn->setMenu(importMenu);
    btn->setPopupMode(QToolButton::InstantPopup);
    //btn->setArrowType(Qt::LeftArrow);
    btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    ui->mainToolBar->addWidget(btn);

    // Load the application and logic
    if (!LoadDatabaseAndUI())
    {
        m_shouldExit = true;
        return;
    }

    // Initialize important controls
    ui->bv->Initialize(&dbm, BookmarksView::LM_FullInformationAndEdit, &dbm.bms.model);
    connect(ui->bv, SIGNAL(activated(long long)), this, SLOT(bvActivated(long long)));
    connect(ui->bv, SIGNAL(currentRowChanged(long long,long long)),
            this, SLOT(bvCurrentRowChanged(long long,long long)));

    //Connecting signal after Initialize()ing tf will miss the first emitted signal, which is
    //  folder '0, Unsorted' being activated. So this relies on a tf behaviour which returns FOID=0
    //  when it's not initalized. TODO: Because of this and also the status labels, I think initing the UI must come after these lines.
    ui->tf->Initialize(&dbm);
    connect(ui->tf, SIGNAL(CurrentFolderChanged(long long)),
            this,   SLOT(tfCurrentFolderChanged(long long)));
    connect(ui->tf, SIGNAL(RequestMoveBookmarksToFolder(QList<long long>,long long)),
            this,   SLOT(tfRequestMoveBookmarksToFolder(QList<long long>,long long)));

    // Additional sub-parts initialization.
    if (!dbm.files.InitializeFileArchives())
    {
        m_shouldExit = true;
        return;
    }
    //The following is not a big deal, we overlook its fails and don't check its return value.
    dbm.files.ClearSandBox();

    qApp->postEvent(this, new QResizeEvent(this->size(), this->size()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::shouldExit() const
{
    return m_shouldExit;
}

void MainWindow::on_btnNew_clicked()
{
    NewBookmark();
}

void MainWindow::on_btnView_clicked()
{
    ViewSelectedBookmark();
}

void MainWindow::on_btnEdit_clicked()
{
    EditSelectedBookmark();
}

void MainWindow::on_btnDelete_clicked()
{
    DeleteSelectedBookmark();
}

void MainWindow::bvActivated(long long BID)
{
    Q_UNUSED(BID);
    ViewSelectedBookmark();
}

void MainWindow::bvCurrentRowChanged(long long currentBID, long long previousBID)
{
    Q_UNUSED(previousBID);
    register bool valid = (currentBID != -1);
    ui->btnView->setEnabled(valid);
    ui->btnEdit->setEnabled(valid);
    ui->btnDelete->setEnabled(valid);
}

void MainWindow::tfCurrentFolderChanged(long long FOID)
{
    Q_UNUSED(FOID);
    RefreshUIDataDisplay(false, RA_Focus);
}

void MainWindow::tfRequestMoveBookmarksToFolder(const QList<long long>& BIDs, long long FOID)
{
    BookmarksBusinessLogic bbLogic(&dbm, this);
    bbLogic.MoveBookmarksToFolderTrans(BIDs, FOID);
    ui->tf->SetCurrentFOIDSilently(FOID);
    //Note [MULTI-BM-SELECT]: This should select ALL of them later.
    RefreshUIDataDisplay(false, RA_CustomSelectAndFocus, BIDs[0]);
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
        CheckAllTags(checkState);
    }
    else
    {
        //A tag item was checked/unchecked. Set the check state of the "All Tags" item.
        QueryAllTagsChecked();
        UpdateAllTagsCheckBoxCheck();
    }

    //Connect the signal we disconnected.
    connect(ui->lwTags, SIGNAL(itemChanged(QListWidgetItem*)),
            this, SLOT(lwTagsItemChanged(QListWidgetItem*)));

    //Now filter the bookmarks according to those tags.
    //Note: Instead of just `RefreshTVBookmarksModelView()` we use the full Refresh below.
    //  It's more standard for this task; we want to save selection, and also update status labels.
    //  We don't populate the data again though.
    //20141009: A new save selection method now saves selection upon filtering by tags.
    //  Read the comments in the following function at the place where selection is restored.
    RefreshUIDataDisplay(false, RA_SaveSelAndFocus, -1,
                         (RefreshAction)(RA_SaveSelAndScroll | RA_NoRefreshView));
}

void MainWindow::on_action_importFirefoxBookmarks_triggered()
{
    //TODO [IMPORT]
}

void MainWindow::on_actionImportFirefoxBookmarksJSONfile_triggered()
{
    QString jsonFilePath = QFileDialog::getOpenFileName(
                this, "Import Firefox Bookmarks Backup JSON File", QString(),
                "Firefox Bookmarks Backup (bookmarks*.json)");

    if (jsonFilePath.isEmpty())
        return;

    ImportFirefoxJSONFile(jsonFilePath);
}

bool MainWindow::LoadDatabaseAndUI()
{
    bool success;

    QString databaseFilePath = QDir::currentPath() + "/" + conf.programDatabasetFileName;
    success = dbm.BackupOpenOrCreate(databaseFilePath);
    if (!success)
        return false;

    RefreshUIDataDisplay(true, RA_Focus);
    return true;
}

void MainWindow::RefreshUIDataDisplay(bool rePopulateModels,
                                      MainWindow::RefreshAction bookmarksAction, long long selectBID,
                                      MainWindow::RefreshAction tagsAction, long long selectTID,
                                      const QList<long long>& newTIDsToCheck)
{
    //Calling this function after changes is needed, even for the bookmarks list.
    //  The model does NOT automatically update its view. Actually calling
    //  `PopulateModelsAndInternalTables` DOES invalidate the selection, etc, but the tableView
    //  is not updated.

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

    int selectedTId = -1;
    int hTagsScrollPos = 0, hBScrollPos = 0, vBScrollPos = 0;

    //Save required things.
    //This is required to be a persistent model for saving selections after filtering.
    QPersistentModelIndex selectedBRowIndex;
    if (bookmarksAction & RA_SaveSel) //[SavingSelectedBookmarkAndTag]
        if (ui->bv->selectionModel()->selectedIndexes().size() > 0)
            //!selectedBRow = ui->tvBookmarks->selectionModel()->selectedIndexes()[0].row();
            selectedBRowIndex = ui->bv->selectionModel()->selectedIndexes()[0];
    if (tagsAction & RA_SaveSel) //[SavingSelectedBookmarkAndTag]
        selectedTId = GetSelectedTagID();
    if (bookmarksAction & RA_SaveScrollPos)
    {
        if (ui->bv->horizontalScrollBar() != NULL)
            hBScrollPos = ui->bv->horizontalScrollBar()->value();
        if (ui->bv->verticalScrollBar() != NULL)
            vBScrollPos = ui->bv->verticalScrollBar()->value();
    }
    if (tagsAction & RA_SaveScrollPos)
        if (ui->lwTags->verticalScrollBar() != NULL)
            hTagsScrollPos = ui->lwTags->verticalScrollBar()->value();
    //ONLY care about check states if there were already tag FILTERations.
    QList<long long> checkedTIDs;
    TagCheckStateResult previousTagsState = m_allTagsChecked;
    if ((tagsAction & RA_SaveCheckState) && (previousTagsState == TCSR_SomeChecked))
        checkedTIDs  = GetCheckedTIDs();

    //Real updating here
    if (rePopulateModels)
        dbm.PopulateModelsAndInternalTables();

    if (!(tagsAction & RA_NoRefreshView))
        RefreshTagsDisplay();

    //Now make sure those we want to check are checked.
    //IMPORTANT: Must do it early, as the next function is immediately `RefreshTVBookmarksModelView`
    //  which RELIES on the checks.
    if (tagsAction & RA_SaveCheckState)
    {
        //If either None or Some or All tags are checked, we need to preserve their check state.
        //  Also if a new tag is added in this situation, it needs to become Checked so that it's
        //  visible in the tags list, using newTIDsToCheck.
        if (previousTagsState == TCSR_NoneChecked)
        {
            //Leave them unchecked
            //No need to CheckAllTags(checkState); Just update our variable:
            m_allTagsChecked = TCSR_NoneChecked;
        }
        else if (previousTagsState == TCSR_SomeChecked)
        {
            //Only check these new things if our list was already filtered by tags.
            //New TIDs are already added to `tagItems` as the tags are refreshed above.
            //  So it's safe that the following function references `tagItems[newCheckedTID]`.
            //  Also we are checking additional items ONLY IF RA_SaveCheckState is set.
            RestoreCheckedTIDs(checkedTIDs, newTIDsToCheck);
        }
        else if (previousTagsState == TCSR_AllChecked)
        {
            CheckAllTags(Qt::Checked);
        }
    }

    //Technically we only need to forceResetFilter only when adding and deleting a bookmark, so we
    //  could have e.g a new action called `RA_ForceResetFilter`; but for simplicity we force
    //  resetting the filter on every change, including the unneeded startup and editing actions.
    if (!(bookmarksAction & RA_NoRefreshView))
        RefreshTVBookmarksModelView(rePopulateModels);

    //Refresh status labels.
    RefreshStatusLabels();

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
    if (bookmarksAction & RA_SaveSel)
        ui->bv->setCurrentIndex(selectedBRowIndex);
        //if (selectedBRow != -1)
            //!ui->tvBookmarks->setCurrentIndex(filteredBookmarksModel.index(selectedBRow, 0));
    if (tagsAction & RA_SaveSel)
        if (selectedTId != -1)
            SelectTagWithID(selectedTId);
    if (bookmarksAction & RA_SaveScrollPos)
    {
        if (ui->bv->horizontalScrollBar() != NULL)
            ui->bv->horizontalScrollBar()->setValue(hBScrollPos);
        if (ui->bv->verticalScrollBar() != NULL)
            ui->bv->verticalScrollBar()->setValue(vBScrollPos);
    }
    if (tagsAction & RA_SaveScrollPos)
        if (ui->lwTags->verticalScrollBar() != NULL)
            ui->lwTags->verticalScrollBar()->setValue(hTagsScrollPos);

    //[RestoringScrollPositionProceedsCustomSelection]
    if (bookmarksAction & RA_CustomSelect)
        if (selectBID != -1)
            ui->bv->SelectBookmarkWithID(selectBID);
    if (tagsAction & RA_CustomSelect)
        if (selectTID != -1)
            SelectTagWithID(selectTID);

    //Focusing comes last anyway
    if (bookmarksAction & RA_Focus)
        ui->bv->setFocus();
    if (tagsAction & RA_Focus)
        ui->lwTags->setFocus();

    //Connect the signal we disconnected.
    connect(ui->lwTags, SIGNAL(itemChanged(QListWidgetItem*)),
            this, SLOT(lwTagsItemChanged(QListWidgetItem*)));
}

void MainWindow::RefreshStatusLabels()
{
    bool allTagsOrNoneOfTheTags = (m_allTagsChecked == TCSR_AllChecked
                                || m_allTagsChecked == TCSR_NoneChecked);

    if (allTagsOrNoneOfTheTags)
    {
        ui->lblFilter->setText("Showing bookmarks");
    }
    else
    {
        QString checkedTagsNameList;
        foreach (QListWidgetItem* tagItem, tagItems) //Values
            if (tagItem->checkState() == Qt::Checked)
                checkedTagsNameList += tagItem->text() + ", ";
        //We are sure there was at least one tag selected.
        checkedTagsNameList.chop(2);

        ui->lblFilter->setText("Showing bookmarks tagged <span style=\"color:green;\">"
                               + checkedTagsNameList + "</span>");
    }

    QString currentFolder = dbm.bfs.bookmarkFolders[ui->tf->GetCurrentFOID()].Ex_AbsolutePath;
    if (currentFolder.isEmpty()) //For the '0, Unsorted' folder the path is empty.
        currentFolder =  dbm.bfs.bookmarkFolders[ui->tf->GetCurrentFOID()].Name;
    currentFolder = " in folder <span style=\"color:blue;\">" + currentFolder + "</span>";
    ui->lblFilter->setText(ui->lblFilter->text() + currentFolder);

    ui->lblBMCount->setText(QString("%1/%2 Bookmarks")
                            .arg(ui->bv->GetDisplayedBookmarksCount())
                            .arg(ui->bv->GetTotalBookmarksCount()));
}

void MainWindow::RefreshTVBookmarksModelView(bool forceResetFilter)
{
    //First we check if "All Items" is checked or not. If it's fully checked or fully unchecked,
    //  we don't filter by tags.
    bool allTagsOrNoneOfTheTags = (m_allTagsChecked == TCSR_AllChecked
                                || m_allTagsChecked == TCSR_NoneChecked);

    BookmarkFilter bfilter;

    bfilter.FilterSpecificFolderIDs(QSet<long long>() << ui->tf->GetCurrentFOID());

    if (allTagsOrNoneOfTheTags)
    {
        //Let bfilter stay clear (actually it's not clear; it has filtered folders already).
    }
    else
    {
        //"All Items" does not appear in `tagItems`. Even if it was, it was excluded by the Qt::Checked
        //  condition, as we have already checked `allTagsOrNoneOfTheTags`.
        QSet<long long> tagIDs;
        for (auto it = tagItems.constBegin(); it != tagItems.constEnd(); ++it)
            if (it.value()->checkState() == Qt::Checked)
                tagIDs.insert(it.key());

        bfilter.FilterSpecificTagIDs(tagIDs);
    }

    ui->bv->SetFilter(bfilter, forceResetFilter);
    ui->bv->RefreshView();
}

void MainWindow::NewBookmark()
{
    //I hesitated at allocating the modal dialog on stack, but look here for a nice discussion
    //  about message loop of modal dialogs:
    //  http://qt-project.org/forums/viewthread/15361

    BookmarkEditDialog::OutParams outParams;
    BookmarkEditDialog bmEditDialog(&dbm, -1, ui->tf->GetCurrentFOID(), &outParams, this);

    if (!bmEditDialog.canShow())
        return; //In case of errors a message is already shown.

    int result = bmEditDialog.exec();
    if (result != QDialog::Accepted)
        return;

    RefreshUIDataDisplay(true, RA_CustomSelectAndFocus, outParams.addedBId, RA_SaveSelAndScrollAndCheck,
                         -1, outParams.associatedTIDs);
}

void MainWindow::ViewSelectedBookmark()
{
    BookmarkViewDialog bmViewDialog(&dbm, ui->bv->GetSelectedBookmarkID(), this);

    if (!bmViewDialog.canShow())
        return; //In case of errors a message is already shown.

    bmViewDialog.exec();

    //No need to refresh UI display.
}

void MainWindow::EditSelectedBookmark()
{
    BookmarkEditDialog::OutParams outParams;
    const long long BID = ui->bv->GetSelectedBookmarkID();
    BookmarkEditDialog bmEditDialog(&dbm, BID, -1, &outParams, this);

    if (!bmEditDialog.canShow())
        return; //In case of errors a message is already shown.

    int result = bmEditDialog.exec();
    if (result != QDialog::Accepted)
        return;

    //20141009: The model is reset so we should do the selection manually ourselves.
    RefreshUIDataDisplay(true, RA_CustomSelAndSaveScrollAndFocus, BID, RA_SaveSelAndScrollAndCheck,
                         -1, outParams.associatedTIDs);
}

void MainWindow::DeleteSelectedBookmark()
{
    QString selectedBookmarkName = ui->bv->GetSelectedBookmarkName();

    if (QMessageBox::Yes !=
        QMessageBox::question(this, "Delete Bookmark",
                              "Are you sure you want to send the selected bookmark \""
                              + selectedBookmarkName + "\" to the trash?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
        return;

    BookmarksBusinessLogic bbLogic(&dbm, this);
    bool success = bbLogic.DeleteBookmarkTrans(ui->bv->GetSelectedBookmarkID());
    if (!success)
        return;

    //After delete, no item remains selected.
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

void MainWindow::QueryAllTagsChecked()
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
        m_allTagsChecked = TCSR_SomeChecked;
    else if (seenChecked) // && !seenUnchecked
        m_allTagsChecked = TCSR_AllChecked;
    else if (seenUnchecked) // && !seenChecked
        m_allTagsChecked = TCSR_NoneChecked;
    else //Empty tag items.
        m_allTagsChecked = TCSR_NoneChecked;
}

void MainWindow::UpdateAllTagsCheckBoxCheck()
{
    //Note: `RefreshTVBookmarksModelView` relies on this.
    if (m_allTagsChecked == TCSR_NoneChecked)
        ui->lwTags->item(0)->setCheckState(Qt::Unchecked);
    else if (m_allTagsChecked == TCSR_SomeChecked)
        ui->lwTags->item(0)->setCheckState(Qt::PartiallyChecked);
    else if (m_allTagsChecked == TCSR_AllChecked)
        ui->lwTags->item(0)->setCheckState(Qt::Checked);
}

void MainWindow::CheckAllTags(Qt::CheckState checkState)
{
    //First do the `tagItems`.
    foreach (QListWidgetItem* item, tagItems.values())
        item->setCheckState(checkState);

    //IMPORTANT: Update our variable.
    //Note: It was standard to call the QueryAllTagsChecked(); function here, but that
    //  meant an additional `for` loop. So we do it directly here.
    m_allTagsChecked = (checkState == Qt::Checked
                        ? TCSR_AllChecked
                        : TCSR_NoneChecked);

    //Additionally set the check state for 'All Tags', too; although this function is also called
    //  from its event handler and we don't need it in that case.
    ui->lwTags->item(0)->setCheckState(checkState);
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

    QueryAllTagsChecked();
    UpdateAllTagsCheckBoxCheck();
}

void MainWindow::ImportFirefoxJSONFile(const QString& jsonFilePath)
{
    bool success;
    ImportedEntityList elist;
    elist.importSource = ImportedEntityList::Source_Firefox;
    elist.importSourceFileName = QFileInfo(jsonFilePath).fileName();

    FirefoxBookmarkJSONFileParser ffParser(this, &conf);
    success = ffParser.ParseFile(jsonFilePath, elist);
    if (!success)
        return;

    BookmarkImporter bmim(&dbm, this);
    success = bmim.Initialize();
    if (!success)
        return;

    success = bmim.Analyze(elist);
    if (!success)
        return;

    //Note about TRANSACTIONS:
    //Import function imports each bookmark in its own transaction. This way is both just what we
    //  did (i.e importing all of them under the same transaction was not more difficult and could
    //  be done), and also the good point about it is that if it interrupts, we'll have some of our
    //  bookmarks. So we don't need to start and wrap this in a transaction.
    success = bmim.InitializeImport();
    if (!success)
        return;

    ImportedBookmarksPreviewDialog* importPreviewDialog = new ImportedBookmarksPreviewDialog(&dbm, &bmim, &elist, this);
    success = importPreviewDialog->canShow();
    if (!success)
        return;

    int result = importPreviewDialog->exec();
    importPreviewDialog->deleteLater();

    //Don't return if rejected. Even if rejected some bookmarks may have been imported.
    //if (result != QDialog::Accepted)
    //    return;
    Q_UNUSED(result);

    QList<long long> addedBIDs;
    QSet<long long> allAssociatedTIDs;
    QList<ImportedBookmark*> failedProcessOrImports;
    bmim.FinalizeImport(addedBIDs, allAssociatedTIDs, failedProcessOrImports);

    ///Previously the preview dialog didn't import bookmarks, but now it imports them using its
    ///  bookmarks processor. So we had to import the bookmarks after it was accepted here.
    //success = bmim.Import(elist, addedBIDs, allAssociatedTIDs);
    //if (!success)
    //    return;

    //Refresh UI
    if (!addedBIDs.isEmpty())
    {
        //Show '0, Unsorted' into which the bookmarks are imported.
        ui->tf->SetCurrentFOIDSilently(0);
        //Note [MULTI-BM-SELECT]: Select the whole list later.
        RefreshUIDataDisplay(true, RA_CustomSelectAndFocus, addedBIDs[0],
                RA_SaveSelAndScrollAndCheck, -1, allAssociatedTIDs.toList());
    }

    //Show success message
    QString importCompleteMessage;
    if (addedBIDs.isEmpty())
        importCompleteMessage = "No bookmarks were imported.";
    else
        importCompleteMessage = QString("%1 bookmark(s) were imported.").arg(addedBIDs.size());
    if (!failedProcessOrImports.isEmpty())
        importCompleteMessage += "\n\nSome bookmarks encountered errors while importing. "
                                 "Click OK to view the error messages.";
    QMessageBox::information(this, "Import complete", importCompleteMessage);

    if (!failedProcessOrImports.isEmpty())
    {
        QString errorMessages;
        foreach (ImportedBookmark* ib, failedProcessOrImports)
            errorMessages += QString("%1 (%2):\n%3\n\n").arg(ib->title, ib->uri, ib->ExIm_finalError);
        QPlainTextEdit* ptxErrors = new QPlainTextEdit(NULL /* no parent : makes it top-level window. */);
        ptxErrors->setWindowTitle("Bookmark Processing and Import Errors");
        ptxErrors->setPlainText(errorMessages);
        ptxErrors->resize(this->size() * 0.75);
        ptxErrors->show();


        /*QRect geom = geometry();
        geom.moveCenter(QApplication::desktop()->availableGeometry().center());
        this->setGeometry(geom);*/
    }
}

#include "MHTSaver.h"

class MHTDataReceiver : public QObject
{
    Q_OBJECT
public:
    MHTDataReceiver(QObject* parent = NULL) : QObject(parent) { }
    ~MHTDataReceiver() { qDebug() << "MHTDataWritten"; }
public slots:
    void MHTDataReady(const QByteArray& data, const MHTSaver::Status& status)
    {
        Q_UNUSED(status);
        qDebug() << "MHTDataReady";

        QFile mhtfile("C:\\Users\\Hossein\\Desktop\\LastMHT.mht");
        mhtfile.open(QIODevice::WriteOnly);
        mhtfile.write(data);
        mhtfile.close();

        sender()->deleteLater();
        this->deleteLater();
    }
};
#include "MainWindow.moc" //Just for the above test mht to work (moc_MainWindow.cpp didn't work btw)

void MainWindow::on_actionGetMHT_triggered()
{
    MHTSaver* saver = new MHTSaver(this);
    saver->GetMHTData("https://www.mozilla.org/en-US/firefox/new/");
    MHTDataReceiver* datarecv = new MHTDataReceiver(this);
    connect(saver, SIGNAL(MHTDataReady(QByteArray,MHTSaver::Status)), datarecv, SLOT(MHTDataReady(QByteArray,MHTSaver::Status)));
}
