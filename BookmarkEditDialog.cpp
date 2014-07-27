#include "BookmarkEditDialog.h"
#include "ui_BookmarkEditDialog.h"

#include "FileManager.h"

#include <QtGui/QFileDialog>

BookmarkEditDialog::BookmarkEditDialog(DatabaseManager* dbm,
                                       long long editBId, QWidget *parent) :
    QDialog(parent), ui(new Ui::BookmarkEditDialog), dbm(dbm),
    canShowTheDialog(false), editBId(editBId)
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
    //TODO: bdata.FileName = ui->leFileName->text().trimmed();
    bdata.Desc = ui->ptxDesc->toPlainText();
    //bdata.DefFile = TODO
    bdata.Rating = ui->dialRating->value();

    QStringList tagsList = ui->leTags->text().split(' ', QString::SkipEmptyParts);

    bool success;

    //TODO: Manage tags and files

    dbm->db.transaction();
    {
        success = dbm->bms.AddOrEditBookmark(editBId, bdata); //For Add, the editBID will be modified!
        if (!success)
        {
            dbm->db.rollback();
            return;
        }

        success = dbm->tags.SetBookmarkTags(editBId, tagsList);
        if (!success)
        {
            dbm->db.rollback();
            return;
        }
    }
    dbm->db.commit();


/*TODO:
    if (editBId != -1 && editOriginalBData.FileName == bdata.FileName)
    {
        //Do nothing with files
    }
    //TODO: Must detect here if user has removed a ":archive:" url in the textbox and do sth with its file!
    else if (!bdata.FileName.isEmpty())
    {
        //Remove possible extra quotes.
        if (bdata.FileName.left(1) == "\"" && bdata.FileName.right(1) == "\"")
            bdata.FileName = bdata.FileName.mid(1, bdata.FileName.length() - 2);

        if (fam->IsInsideFileArchive(bdata.FileName))
        {
            //TODO: What if the user changes the file to another archive file? We need refcounting here?
            //TODO: Check if changed and then add to database.
            //TODO: Support multiple file attaching.
        }
        else
        {
            //Copy/Move into the file archive.
            bool removeOriginal = ui->chkRemoveOriginalFile->isChecked();
            QString NewFileName = bdata.FileName;
            bool success = fam->PutInsideFileArchive(NewFileName, removeOriginal);
            if (success)
                bdata.FileName = NewFileName;
            else
                return; //Don't accept the dialog.
        }
    }
    bool success = dbm->bms.AddOrEditBookmark(editBId, bdata);
*/

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
    //TODO: DefFile

    foreach (const FileManager::BookmarkFile& bf, editedFilesList)
    {
        int rowIdx = ui->twAttachedFiles->rowCount();
        ui->twAttachedFiles->insertRow(rowIdx);

        QTableWidgetItem* nameItem = new QTableWidgetItem(bf.OriginalName);
        //TODO: Show reasonable size.
        QTableWidgetItem* sizeItem = new QTableWidgetItem(QString::number(bf.Size));

        ui->twAttachedFiles->setItem(rowIdx, 0, nameItem);
        ui->twAttachedFiles->setItem(rowIdx, 1, sizeItem);
    }

    ui->twAttachedFiles->resizeColumnsToContents();

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

    NOW BOLD AND HIGHLIGHT THE DefFile.
    */
}

void BookmarkEditDialog::on_btnShowAttachUI_clicked()
{
    ui->stwFileAttachments->setCurrentWidget(ui->pageAttachNew);
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
    QStringList list = QFileDialog::getOpenFileNames(this, "Select File(s)", QString(),
                                                     filters.join(";;"), &filters.last());

    //Docs say iterate on copies.
    QStringList fileNames = list;

    if (fileNames.isEmpty())
        return;

    foreach (const QString& fileName, fileNames)
    {
        //TODO HANDLE THE THINGS BELOW:
        //Original file Information MUST be filled by us.
        //  FileManager will take care of the IDs and archive url, etc.
        FileManager::BookmarkFile bf;
        bf.BFID         = -1;
        bf.BID          = editBId; //TODO: Handle adding files
        bf.FID          = -1; //TODO: Handle.
        bf.OriginalName = fileName;
        bf.ArchiveURL   = ""; //TODO: Handle.
        bf.ModifyDate   = QDateTime::currentDateTime();
        bf.Size         = rand();
        bf.MD5          = "query.value(bfidx.MD5         ).toString();";

        editedFilesList.append(bf);
    }

    PopulateUIFiles(true);
}

void BookmarkEditDialog::on_btnAttach_clicked()
{
    //TODO
}

void BookmarkEditDialog::on_btnCancelAttach_clicked()
{
    ui->stwFileAttachments->setCurrentWidget(ui->pageAttachedFiles);
}
