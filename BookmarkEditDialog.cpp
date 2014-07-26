#include "BookmarkEditDialog.h"
#include "ui_BookmarkEditDialog.h"

#include "FileManager.h"

BookmarkEditDialog::BookmarkEditDialog(DatabaseManager* dbm,
                                       long long editBId, QWidget *parent) :
    QDialog(parent), ui(new Ui::BookmarkEditDialog), dbm(dbm),
    canShowTheDialog(false), editBId(editBId)
{
    ui->setupUi(this);
    ui->leTags->setModel(&dbm->tags.model);
    ui->leTags->setModelColumn(dbm->tags.tidx.TagName);

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

        canShowTheDialog = dbm->files.RetrieveBookmarkFilesModel(editBId, editOriginalBData.Ex_FilesModel);
        if (!canShowTheDialog)
            return;

        //Show in the UI.
        ui->leName    ->setText(editOriginalBData.Name);
        ui->leURL     ->setText(editOriginalBData.URL);
        ui->ptxDesc   ->setPlainText(editOriginalBData.Desc);
        ui->dialRating->setValue(editOriginalBData.Rating);
        PopulateUITags();
        PopulateUIFiles();
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

    //TODO: Manage tags and files
    //bdata.Tags = ui->leTags->text().trimmed();
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
*/
    bool success = dbm->bms.AddOrEditBookmark(editBId, bdata);
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

void BookmarkEditDialog::PopulateUIFiles()
{
    //TODO: DefFile
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
}

void BookmarkEditDialog::on_btnShowAttachUI_clicked()
{
    ui->stwFileAttachments->setCurrentWidget(ui->pageAttachNew);
}

void BookmarkEditDialog::on_tvAttachedFiles_activated(const QModelIndex &index)
{

}

void BookmarkEditDialog::tvAttachedFilesCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{

}

void BookmarkEditDialog::on_tvAttachedFiles_customContextMenuRequested(const QPoint &pos)
{

}

void BookmarkEditDialog::on_btnBrowse_clicked()
{

}

void BookmarkEditDialog::on_btnAttach_clicked()
{

}

void BookmarkEditDialog::on_btnCancelAttach_clicked()
{
    ui->stwFileAttachments->setCurrentWidget(ui->pageAttachedFiles);
}
