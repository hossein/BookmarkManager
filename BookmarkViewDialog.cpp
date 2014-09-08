#include "BookmarkViewDialog.h"
#include "ui_BookmarkViewDialog.h"

#include "Util.h"

BookmarkViewDialog::BookmarkViewDialog(DatabaseManager* dbm, long long viewBId, QWidget *parent) :
    QDialog(parent), ui(new Ui::BookmarkViewDialog),
    dbm(dbm), canShowTheDialog(false)
{
    ui->setupUi(this);
    ui->leTags->setModel(&dbm->tags.model);
    ui->leTags->setModelColumn(dbm->tags.tidx.TagName);

    ui->fvsRating->setStarSize(ui->lblName->sizeHint().height());

    InitializeFilesUI();

    canShowTheDialog = dbm->bms.RetrieveBookmark(viewBId, viewBData);
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

    //Additional bookmark variables.
    SetDefaultBFID(viewBData.DefBFID); //Needed; retrieving functions don't set this.
    int defFileIndex = DefaultFileIndex();
    if (defFileIndex != -1)
        PreviewFile(defFileIndex);

    //Show in the UI.
    setWindowTitle("View Bookmark: " + viewBData.Name);
    ui->lblName   ->setText(viewBData.Name);
    ui->lblName   ->setToolTip(viewBData.Name);
    ui->fvsRating ->setValue(viewBData.Rating);
    ui->fvsRating ->setToolTip(QString::number(viewBData.Rating / (double)10.0));
    ui->ptxDesc   ->setPlainText(viewBData.Desc);
    ui->leURL     ->setText(viewBData.URL);
    PopulateUITags();
    PopulateUIFiles(false); //TODO: Single-Click to show the file. Also i think DefFile must be selected too?
                            //TODO: Also context menu for these files.

}

BookmarkViewDialog::~BookmarkViewDialog()
{
    delete ui;
}

bool BookmarkViewDialog::canShow()
{
    return canShowTheDialog;
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
            fileName = dbm->files.GetUserReadableArchiveFilePath(bf.OriginalName);
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

    ui->twAttachedFiles->resizeColumnsToContents();

    if (saveSelection && selectedRow != -1)
        if (selectedRow < ui->twAttachedFiles->rowCount())
            ui->twAttachedFiles->selectRow(selectedRow);

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

void BookmarkViewDialog::PreviewFile(int index)
{
    QString fileArchiveURL = viewBData.Ex_FilesList[index].ArchiveURL;
    QString realFilePathName = dbm->files.GetFullArchiveFilePath(fileArchiveURL);
    dbm->fview.Preview(realFilePathName, ui->widPreviewer);
}
