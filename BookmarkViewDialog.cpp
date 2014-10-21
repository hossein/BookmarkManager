#include "BookmarkViewDialog.h"
#include "ui_BookmarkViewDialog.h"

#include "Util.h"

#include <QApplication>
#include <QScrollBar>
#include <QMenu>

#include <QUrl>
#include <QDesktopServices>

BookmarkViewDialog::BookmarkViewDialog(DatabaseManager* dbm, Config* conf, long long viewBId, QWidget *parent) :
    QDialog(parent), ui(new Ui::BookmarkViewDialog),
    dbm(dbm), conf(conf), canShowTheDialog(false)
{
    ui->setupUi(this);

    //Maximize button would be more useful than a Help button on this dialog, e.g to view big pictures.
    Qt::WindowFlags flags = this->windowFlags();
    flags &= ~Qt::WindowContextHelpButtonHint;
    flags |= Qt::WindowMaximizeButtonHint;
    this->setWindowFlags(flags);

    ui->leTags->setModel(&dbm->tags.model);
    ui->leTags->setModelColumn(dbm->tags.tidx.TagName);

    ui->fvsRating->setStarSize(ui->lblName->sizeHint().height());

    //Without this the scroll area resized to less than the minimum width of what it contains.
    //NOTE: On Qt 5.2+ maybe we can use the sizeAdjustPolicy property.
    ui->scrlFileData->setMinimumSize(ui->scrlFileDataWidgetContents->minimumSizeHint());

    InitializeFilesUI();
    InitializeLinkedBookmarksUI();

    canShowTheDialog = dbm->bms.RetrieveBookmark(viewBId, viewBData);
    if (!canShowTheDialog)
        return;

    canShowTheDialog = dbm->bms.RetrieveLinkedBookmarks(viewBId, viewBData.Ex_LinkedBookmarksList);
    if (!canShowTheDialog)
        return;

    canShowTheDialog = dbm->tags.RetrieveBookmarkTags(viewBId, viewBData.Ex_TagsList);
    if (!canShowTheDialog)
        return;

    //[No-File-Model-Yet]
    //Note: We don't retrieve the files model and use custom QList's and QTableWidget instead.
    canShowTheDialog = dbm->files.RetrieveBookmarkFiles(viewBId, viewBData.Ex_FilesList);
    if (!canShowTheDialog)
        return;

    if (viewBData.Ex_FilesList.size() == 0)
    {
        //No files: Remove the files group.
        ui->grpFiles->close();

        //We remove widPreviewer and try to reclaim the space it frees.
        ui->widPreviewer->close();

        //Doesn't work:
        //this->resize(this->width(), this->height() - ui->widPreviewer->height());

        //With the assumption that ui->widTopPane has fixed height, we set fixed height for
        //  our dialog too:
        int thisMargins = this->contentsMargins().top() + this->contentsMargins().bottom();
        int layoutMargins = ui->mainVerticalLayout->contentsMargins().top()
                          + ui->mainVerticalLayout->contentsMargins().bottom();
        this->setFixedHeight(ui->widTopPane->height() + thisMargins + layoutMargins);
    }

    if (viewBData.Ex_LinkedBookmarksList.size() == 0)
    {
        ui->grpRelatedBookmarks->close();
    }

    if (viewBData.Ex_FilesList.size() == 0 && viewBData.Ex_LinkedBookmarksList.size() == 0)
    {
        ui->scrlFileData->close();
    }

    //Additional bookmark variables.
    SetDefaultBFID(viewBData.DefBFID); //Needed; retrieving functions don't set this.

    //Show in the UI.
    setWindowTitle("View Bookmark: " + viewBData.Name);
    ui->lblName   ->setText(viewBData.Name);
    ui->lblName   ->setToolTip(viewBData.Name);
    ui->fvsRating ->setValue(viewBData.Rating);
    ui->fvsRating ->setToolTip(QString::number(viewBData.Rating / (double)10.0));
    ui->ptxDesc   ->setPlainText(viewBData.Desc);
    ui->leURL     ->setText(viewBData.URL);
    PopulateUITags();
    PopulateUIFiles(false);
    PopulateLinkedBookmarks();

    //Select the default file, preview it.
    //`DefaultFileIndex()` must be called AFTER the `SetDefaultBFID(...)` call above.
    int defFileIndex = DefaultFileIndex();
    if (defFileIndex != -1)
    {
        ui->twAttachedFiles->selectRow(defFileIndex);
        //The above line causes: PreviewFile(defFileIndex);
    }

    //Needed to make sure if the file list needs a horizontal scrollbar it will have its room.
    qApp->postEvent(this, new QResizeEvent(this->size(), this->size()));
}

BookmarkViewDialog::~BookmarkViewDialog()
{
    delete ui;
}

void BookmarkViewDialog::resizeEvent(QResizeEvent* event)
{
    //Do BEFORE anything.
    QDialog::resizeEvent(event);

    //TODO: But there is still a 1px error! Maybe should calculate column width ourselves or catch tw's resize event?
    /** int widthForAllColumns = 0;
    for (int i = 0; i < ui->twAttachedFiles->columnCount(); i++)
        widthForAllColumns += ui->twAttachedFiles->columnWidth(i);
    qDebug() << widthForAllColumns << ui->twAttachedFiles->frameSize() << ui->twAttachedFiles->sizeHint() << ui->twAttachedFiles->size();*/

    //Don't use `ui->twAttachedFiles->isVisible()` as condition, maybe it's destroyed.
    if (viewBData.Ex_FilesList.size() > 0)
    {
        //We don't need to sum column widths and compare to size, etc.
        int twAttachedFilesHeight = twAttachedFilesRequiredHeight;
        if (ui->twAttachedFiles->horizontalScrollBar()->isVisible())
            //NOT += ui->twAttachedFiles->horizontalScrollBar()->height(). It's 30!
            twAttachedFilesHeight += ui->twAttachedFiles->horizontalScrollBar()->sizeHint().height();
        ui->twAttachedFiles->setFixedHeight(twAttachedFilesHeight);
    }
}

bool BookmarkViewDialog::canShow()
{
    return canShowTheDialog;
}

void BookmarkViewDialog::on_twAttachedFiles_itemActivated(QTableWidgetItem* item)
{
    Q_UNUSED(item);
    af_open();
}

void BookmarkViewDialog::on_twAttachedFiles_itemSelectionChanged()
{
    //This [ignores empty selections], so if all items are unselected (e.g because of the
    //  context menu) the preview is still there. Note: The selection is no more cleared
    //  on any kind of right-click or context menu showin.
    if (!ui->twAttachedFiles->selectedItems().isEmpty())
        PreviewFile(ui->twAttachedFiles->selectedItems()[0]->row());
}

void BookmarkViewDialog::on_twAttachedFiles_customContextMenuRequested(const QPoint& pos)
{
    //NO [Clear selection on useless right-click], i.e we don't clear the selection,
    //  as we do NOT want to show a menu for the cases when NO file is selected.
    //  We just return.
    //I also think the first condition of the following `if` implies the second (so second not needed)
    //  because if there is any item under the mouse when right-clicking it will already be selected
    //  by the time slot is called.
    if (ui->twAttachedFiles->itemAt(pos) == NULL ||
        ui->twAttachedFiles->selectedItems().empty())
        return;

    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    QString filePathName = GetAttachedFileFullPathName(filesListIdx);

    typedef QKeySequence QKS;
    QMenu afMenu("Attached File Menu");

    QAction* a_open     = afMenu.addAction("&Open"           , this, SLOT(af_open()),       QKS("Enter"));
    QAction* a_edit     = afMenu.addAction("Open (&Editable)", this, SLOT(af_edit()));
    QMenu*   m_openWith = afMenu.addMenu  ("Open Wit&h"     );
    dbm->fview.PopulateOpenWithMenu(filePathName, m_openWith , this, SLOT(af_openWith()));
                          afMenu.addSeparator();
    QAction* a_props    = afMenu.addAction("P&roperties"     , this, SLOT(af_properties()), QKS("Alt+Enter"));

    Q_UNUSED(a_edit);
    Q_UNUSED(a_props);

    afMenu.setDefaultAction(a_open); //Always Open is the default double-click action.

    QPoint menuPos = ui->twAttachedFiles->viewport()->mapToGlobal(pos);
    afMenu.exec(menuPos);
}

void BookmarkViewDialog::on_btnOpenUrl_clicked()
{
    QDesktopServices::openUrl(QUrl(viewBData.URL));
}

void BookmarkViewDialog::PopulateUITags()
{
    ui->leTags->setText(viewBData.Ex_TagsList.join(" "));
}

void BookmarkViewDialog::InitializeFilesUI()
{
    ui->twAttachedFiles->setColumnCount(2);
    ui->twAttachedFiles->setHorizontalHeaderLabels(QString("File Name,Size").split(','));

    QHeaderView* hh = ui->twAttachedFiles->horizontalHeader();
    hh->setResizeMode(QHeaderView::ResizeToContents);
    //hh->setResizeMode(0, QHeaderView::Stretch);
    //hh->setResizeMode(1, QHeaderView::Fixed  );
    hh->resizeSection(1, 60);

    QHeaderView* vh = ui->twAttachedFiles->verticalHeader();
    vh->setResizeMode(QHeaderView::ResizeToContents); //Disable changing row height.
}

void BookmarkViewDialog::PopulateUIFiles(bool saveSelection)
{
    //In BookmarkViewDialog, `saveSelection` might be useful in Refreshing the dialog, maybe.

    int selectedRow = -1;
    if (saveSelection)
        if (!ui->twAttachedFiles->selectedItems().empty())
            selectedRow = ui->twAttachedFiles->selectedItems()[0]->row();

    //ui->twAttachedFiles->clear(); No! headers are cleared this way and you have to
    //                              set table dimenstions again
    while (ui->twAttachedFiles->rowCount() > 0)
        ui->twAttachedFiles->removeRow(0);

    int index = 0;
    foreach (const FileManager::BookmarkFile& bf, viewBData.Ex_FilesList)
    {
        int rowIdx = ui->twAttachedFiles->rowCount();
        ui->twAttachedFiles->insertRow(rowIdx);

        QString fileName;
        if (bf.FID == -1)
            fileName = bf.OriginalName;
        else
            fileName = dbm->files.GetUserReadableArchiveFilePath(bf);
        QTableWidgetItem* nameItem = new QTableWidgetItem(fileName);
        QTableWidgetItem* sizeItem = new QTableWidgetItem(Util::UserReadableFileSize(bf.Size));

        //Setting the data to the first item is enough.
        nameItem->setData(Qt::UserRole, index);
        index++;

        if (bf.Ex_IsDefaultFileForEditedBookmark)
        {
            QFont boldFont(this->font());
            boldFont.setBold(true);
            nameItem->setFont(boldFont);
            sizeItem->setFont(boldFont);
        }

        ui->twAttachedFiles->setItem(rowIdx, 0, nameItem);
        ui->twAttachedFiles->setItem(rowIdx, 1, sizeItem);
    }

    if (saveSelection && selectedRow != -1)
        if (selectedRow < ui->twAttachedFiles->rowCount())
            ui->twAttachedFiles->selectRow(selectedRow);

    //Must do before calculating sizes later.
    ui->twAttachedFiles->resizeColumnsToContents();
    ui->twAttachedFiles->resizeRowsToContents();

    //Make the height as small as needed. This size is very exact, at least with Windows style,
    //  i.e the scrollbar appears if it's one pixel less!
    //  Of course we turn off the vertical scrollbar of the twAttachedFiles so it doesn't scroll
    //  on small errors anyway. (We could do it via UI properties too but we are very explicit!)
    //  However user can see small 'jumps' in case of these small errors.
    ui->twAttachedFiles->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    int hackedSuitableHeightForTwAttachedFiles =
            ui->twAttachedFiles->frameWidth() * 2 +
            ui->twAttachedFiles->horizontalHeader()->sizeHint().height() +
            //The row height includes the grid; no need to `rowHeight+1`.
            ui->twAttachedFiles->rowHeight(0) * viewBData.Ex_FilesList.size();

    //Maybe horizontall scroll bar is needed, this way we need to leave room for it.
    //  But we CAN'T know whether it is shown here or not; also the dialog is resizable, so we check for,
    //  and leave room for it if required it in resizeEvent.

    twAttachedFilesRequiredHeight = hackedSuitableHeightForTwAttachedFiles;
    ui->twAttachedFiles->setFixedHeight(twAttachedFilesRequiredHeight);
}

void BookmarkViewDialog::SetDefaultBFID(long long BFID)
{
    //`false` values must also be set.
    for (int i = 0; i < viewBData.Ex_FilesList.size(); i++)
        viewBData.Ex_FilesList[i].Ex_IsDefaultFileForEditedBookmark =
                (viewBData.Ex_FilesList[i].BFID == BFID);
}

int BookmarkViewDialog::DefaultFileIndex()
{
    for (int i = 0; i < viewBData.Ex_FilesList.size(); i++)
        if (viewBData.Ex_FilesList[i].Ex_IsDefaultFileForEditedBookmark)
            return i;
    return -1;
}

void BookmarkViewDialog::af_open()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    dbm->fview.OpenReadOnly(GetAttachedFileFullPathName(filesListIdx), &dbm->files);
}

void BookmarkViewDialog::af_edit()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    dbm->fview.OpenEditable(GetAttachedFileFullPathName(filesListIdx), &dbm->files);
}

void BookmarkViewDialog::af_openWith()
{
    QAction* owitem = qobject_cast<QAction*>(sender());
    if (!owitem)
        return;

    long long SAID = owitem->data().toLongLong();
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    QString filePathName = GetAttachedFileFullPathName(filesListIdx);

    if (SAID == FileViewManager::OWS_OpenWithDialogRequest)
    {
        dbm->fview.OpenWith(filePathName, dbm, this);
    }
    else if (SAID == FileViewManager::OWS_OpenWithSystemDefault)
    {
        dbm->fview.GenericOpenFile(filePathName, -1, true, &dbm->files);
    }
    else
    {
        dbm->fview.GenericOpenFile(filePathName, SAID, true, &dbm->files);
    }
}

void BookmarkViewDialog::af_properties()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    dbm->fview.ShowProperties(GetAttachedFileFullPathName(filesListIdx));
}

QString BookmarkViewDialog::GetAttachedFileFullPathName(int filesListIdx)
{
    //Unlike BookmarkEditDialog, we know all files are attached and are in FileArchive.
    QString fullFilePathName = dbm->files.GetFullArchiveFilePath(
                               viewBData.Ex_FilesList[filesListIdx].ArchiveURL);
    return fullFilePathName;
}

void BookmarkViewDialog::InitializeLinkedBookmarksUI()
{
    ui->bvLinkedBookmarks->Initialize(dbm, conf, BookmarksView::LM_NameOnlyDisplayWithoutHeaders);
    ui->bvLinkedBookmarks->setModel(&dbm->bms.model);
    connect(ui->bvLinkedBookmarks, SIGNAL(activated(long long)),
            this, SLOT(bvLinkedBookmarksActivated(long long)));
}

void BookmarkViewDialog::PopulateLinkedBookmarks()
{
    ui->bvLinkedBookmarks->FilterSpecificBookmarkIDs(viewBData.Ex_LinkedBookmarksList);
    ui->bvLinkedBookmarks->ResetHeadersAndSort();
}

void BookmarkViewDialog::bvLinkedBookmarksActivated(long long BID)
{

}

void BookmarkViewDialog::PreviewFile(int index)
{
    //TODO: The override cursor must show for the rendering of the webkit contents too!
    if (dbm->fview.HasPreviewHandler(viewBData.Ex_FilesList[index].OriginalName))
    {
        QApplication::setOverrideCursor(Qt::BusyCursor);
        {
            QString fileArchiveURL = viewBData.Ex_FilesList[index].ArchiveURL;
            QString realFilePathName = dbm->files.GetFullArchiveFilePath(fileArchiveURL);
            dbm->fview.Preview(realFilePathName, ui->widPreviewer);
        }
        QApplication::restoreOverrideCursor();
    }
    else
    {
        //Don't show the preview of the previous files when the selected file can't be previewed.
        ui->widPreviewer->ClearPreview();
    }
}
