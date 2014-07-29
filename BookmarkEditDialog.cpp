#include "BookmarkEditDialog.h"
#include "ui_BookmarkEditDialog.h"

#include "FileManager.h"
#include "Util.h"

#include <QtGui/QFileDialog>

BookmarkEditDialog::BookmarkEditDialog(DatabaseManager* dbm,
                                       long long editBId, QWidget *parent) :
    QDialog(parent), ui(new Ui::BookmarkEditDialog), dbm(dbm),
    canShowTheDialog(false), editBId(editBId), editedDefBFID(-1)
{
    ui->setupUi(this);
    ui->leTags->setModel(&dbm->tags.model);
    ui->leTags->setModelColumn(dbm->tags.tidx.TagName);

    InitializeFilesUI();

    if (editBId == -1)
    {
        setWindowTitle("Add Bookmark");
        canShowTheDialog = true;
    }
    else
    {
        setWindowTitle("Edit Bookmark");
        //`canShowTheDialog` a.k.a `bool success;`

        canShowTheDialog = dbm->bms.RetrieveBookmark(editBId, editOriginalBData);
        if (!canShowTheDialog)
            return;

        canShowTheDialog = dbm->tags.RetrieveBookmarkTags(editBId, editOriginalBData.Ex_TagsList);
        if (!canShowTheDialog)
            return;

        //Note: We don't retrieve the files model and use custom QList's and QTableWidget instead.
        //canShowTheDialog = dbm->files.RetrieveBookmarkFilesModel(editBId, editOriginalBData.Ex_FilesModel);
        //if (!canShowTheDialog)
        //    return;

        canShowTheDialog = dbm->files.RetrieveBookmarkFiles(editBId, editOriginalBData.Ex_FilesList);
        if (!canShowTheDialog)
            return;

        //Additional bookmark variables.
        editedFilesList = editOriginalBData.Ex_FilesList;
        editedDefBFID = editOriginalBData.DefBFID;

        //Show in the UI.
        ui->leName    ->setText(editOriginalBData.Name);
        ui->leURL     ->setText(editOriginalBData.URL);
        ui->ptxDesc   ->setPlainText(editOriginalBData.Desc);
        ui->dialRating->setValue(editOriginalBData.Rating);
        PopulateUITags();
        PopulateUIFiles(false);
    }
}

BookmarkEditDialog::~BookmarkEditDialog()
{
    delete ui;
}

bool BookmarkEditDialog::canShow()
{
    return canShowTheDialog;
}

void BookmarkEditDialog::accept()
{
    //TODO: If already exists, show warning, switch to the already existent, etc etc
    BookmarkManager::BookmarkData bdata;
    bdata.BID = editBId; //Not important.
    bdata.Name = ui->leName->text().trimmed();
    bdata.URL = ui->leURL->text().trimmed();
    bdata.Desc = ui->ptxDesc->toPlainText();
    bdata.DefBFID = editedDefBFID; //TODO: MAnage single files...
    bdata.Rating = ui->dialRating->value();

    QStringList tagsList = ui->leTags->text().split(' ', QString::SkipEmptyParts);

    bool success;

    //IMPORTANT: In case of RollBack, do NOT return! Must set editBId to its original `-1` value
    //           in case of ADDing. Thanksfully FilesList don't need changing.
    dbm->db.transaction();

    {
        success = dbm->bms.AddOrEditBookmark(editBId, bdata); //For Add, the editBID will be modified!
        if (!success)
        {
            dbm->db.rollback();
            todo this not done.
            editBId = -1;
            return;
        }

        success = dbm->tags.SetBookmarkTags(editBId, tagsList);
        if (!success)
        {
            dbm->db.rollback();
            return;
        }

        //We use the transactions in a nested manner. But 'linear' ('together') was also possible
        //  and easier to understand.
        dbm->files.BeginFilesTransaction();
        success = dbm->files.UpdateBookmarkFiles(editBId, editOriginalBData.Ex_FilesList, editedFilesList);
        if (!success)
        {
            dbm->db.rollback();
            dbm->files.RollBackFilesTransaction();
            return;
        }
        dbm->files.CommitFilesTransaction();
    }
    dbm->db.commit();

    if (success)
        QDialog::accept();
}

void BookmarkEditDialog::on_dialRating_valueChanged(int value)
{
    ui->lblRatingValue->setText(QString::number(value / 10.0, 'f', 1));
}

void BookmarkEditDialog::on_dialRating_sliderMoved(int position)
{
    //This only applies to moving with mouse.
    //ui->dialRating->setValue((position / 10) * 10);
}

void BookmarkEditDialog::on_dialRating_dialReleased()
{
    //This only applies to moving with mouse.
    //Required
    ui->dialRating->setValue((ui->dialRating->value() / 10) * 10);
}

void BookmarkEditDialog::PopulateUITags()
{
    ui->leTags->setText(editOriginalBData.Ex_TagsList.join(" "));
}

void BookmarkEditDialog::InitializeFilesUI()
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

void BookmarkEditDialog::PopulateUIFiles(bool saveSelection)
{
    int selectedRow = -1;
    if (saveSelection)
        if (!ui->twAttachedFiles->selectedItems().empty())
            selectedRow = ui->twAttachedFiles->selectedItems()[0]->row();

    //ui->twAttachedFiles->clear(); No! headers are cleared this way and you have to
    //                              set table dimenstions again
    while (ui->twAttachedFiles->rowCount() > 0)
        ui->twAttachedFiles->removeRow(0);

    int index = 0;
    foreach (const FileManager::BookmarkFile& bf, editedFilesList)
    {
        int rowIdx = ui->twAttachedFiles->rowCount();
        ui->twAttachedFiles->insertRow(rowIdx);

        QString fileName;
        if (bf.FID == -1)
            fileName = bf.OriginalName;
        else
            fileName = dbm->files.makeUserReadableArchiveFilePath(bf.OriginalName);
        QTableWidgetItem* nameItem = new QTableWidgetItem(fileName);
        QTableWidgetItem* sizeItem = new QTableWidgetItem(Util::UserReadableFileSize(bf.Size));

        //Setting the data to the first item is enough.
        nameItem->setData(Qt::UserRole, index);
        index++;

        if (editedDefBFID == bf.BFID)
        {
            QFont boldFont(this->font());
            boldFont.setBold(true);
            nameItem->setFont(boldFont);
            sizeItem->setFont(boldFont);
        }

        ui->twAttachedFiles->setItem(rowIdx, 0, nameItem);
        ui->twAttachedFiles->setItem(rowIdx, 1, sizeItem);
    }

    ui->twAttachedFiles->resizeColumnsToContents();

    if (saveSelection && selectedRow != -1)
        if (selectedRow < ui->twAttachedFiles->rowCount())
            ui->twAttachedFiles->selectRow(selectedRow);

    /* PREVIOUS MODEL-VIEW BASED THING:
    ui->tvAttachedFiles->setModel(&editOriginalBData.Ex_FilesModel);

    QHeaderView* hh = ui->tvAttachedFiles->horizontalHeader();

    FileManager::BookmarkFileIndexes const& bfidx = dbm->files.bfidx;

    //Hide everything except OriginalName and Size.
    hh->hideSection(bfidx.BFID        );
    hh->hideSection(bfidx.BID         );
    hh->hideSection(bfidx.FID         );
  //hh->hideSection(bfidx.OriginalName);
    hh->hideSection(bfidx.ArchiveURL  );
    hh->hideSection(bfidx.ModifyDate  );
  //hh->hideSection(bfidx.Size        );
    hh->hideSection(bfidx.MD5         );

    hh->setResizeMode(bfidx.OriginalName, QHeaderView::Stretch);
    hh->setResizeMode(bfidx.Size        , QHeaderView::Fixed  );
    hh->resizeSection(bfidx.Size, 60);

    QHeaderView* vh = ui->tvAttachedFiles->verticalHeader();
    vh->setResizeMode(QHeaderView::ResizeToContents); //Disable changing row height.

    //This function is just called once from the constructor, so this connection is one-time and fine.
    connect(ui->tvAttachedFiles->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(tvAttachedFilesCurrentRowChanged(QModelIndex,QModelIndex)));

    NOW BOLD AND HIGHLIGHT THE DefBFID.
    */
}

void BookmarkEditDialog::on_btnShowAttachUI_clicked()
{
    ui->stwFileAttachments->setCurrentWidget(ui->pageAttachNew);
}

void BookmarkEditDialog::on_btnSetFileAsDefault_clicked()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    editedDefBFID = editedFilesList[filesListIdx].BFID;
    PopulateUIFiles(true);
    ui->twAttachedFiles->setFocus();
}

void BookmarkEditDialog::on_twAttachedFiles_itemActivated(QTableWidgetItem* item)
{
    //TODO
}

void BookmarkEditDialog::on_twAttachedFiles_itemSelectionChanged()
{
    ui->btnSetFileAsDefault->setEnabled(!(ui->twAttachedFiles->selectedItems().isEmpty()));
}

void BookmarkEditDialog::on_twAttachedFiles_customContextMenuRequested(const QPoint &pos)
{
    //TODO
}

void BookmarkEditDialog::on_btnBrowse_clicked()
{
    QStringList filters;
    filters << "Web Page Files (*.htm; *.html; *.mht; *.mhtml; *.maff)"
            << "All Files (*.*)";

    QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select File(s)", QString(),
                                                          filters.join(";;"), &filters.last());

    ui->leFileName->setText(fileNames.join("|"));
}

void BookmarkEditDialog::on_btnAttach_clicked()
{
    //NOTE: Multiple file handling:
    //  Must detect here if user has removed a ":archive:" url in the textbox and do sth with its file!

    QString allFileNames = ui->leFileName->text().trimmed();
    if (allFileNames.isEmpty())
    {
        QMessageBox::warning(this, "Can't Attach Files", "No file name is specified. "
                             "Please select one or more files for attaching.");
        return;
    }

    QStringList fileNames = allFileNames.split("|", QString::SkipEmptyParts);


    foreach (QString fileName, fileNames)
    {
        //Remove possible extra quotes.
        if (fileName.left(1) == "\"" && fileName.right(1) == "\"")
            fileName = fileName.mid(1, fileName.length() - 2);

        QFileInfo fileInfo(fileName);
        if (!fileInfo.isFile())
        {
            QMessageBox::warning(this, "Can't Attach Files", "The file \"" + fileName + "\" "
                                 "does not exist! It will be skipped and not attached.");
            continue;
        }

        //IMPORTANT:
        //Original file Information MUST be filled by us. We will SET BOTH `BFID` AND `FID` to -1,
        //  for which FileManager will take care of the IDs and archive url, etc.
        //NOTE: BOTH `BFID` and `FID` are CRUCIAL to be set to `-1`!
        //      BFID's value lets FileManager know this is a new file-bookmark attachment.
        //      FID is what in the FileTable and is important for FileManager to determine which
        //      files are new and must be inserted into the FileArchive.
        //IN THE FUTURE, when the interface supports sharing a file between two bookmarks, we will
        //      set BFID=-1 and FID=RealFileID for this purpose.
        FileManager::BookmarkFile bf;
        bf.BFID         = -1; //Leave to FileManager.
        bf.BID          = editBId;
        bf.FID          = -1; //Leave to FileManager.
        bf.OriginalName = fileName;
        bf.ArchiveURL   = ""; //Leave to FileManager.
        bf.ModifyDate   = fileInfo.lastModified();
        bf.Size         = fileInfo.size();
        bf.MD5          = Util::GetMD5HashForFile(fileName);
        bf.Ex_RemoveAfterAttach = ui->chkRemoveOriginalFile->isChecked();

        editedFilesList.append(bf);
    }

    PopulateUIFiles(true);
    ClearAndSwitchToAttachedFilesTab();
}

void BookmarkEditDialog::on_btnCancelAttach_clicked()
{
    ClearAndSwitchToAttachedFilesTab();
}

void BookmarkEditDialog::ClearAndSwitchToAttachedFilesTab()
{
    ui->stwFileAttachments->setCurrentWidget(ui->pageAttachedFiles);
    ui->leFileName->clear();
    ui->chkRemoveOriginalFile->setChecked(false);
    ui->twAttachedFiles->setFocus();
}
