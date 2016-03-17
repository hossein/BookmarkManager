#include "BookmarkFolderEditDialog.h"
#include "ui_BookmarkFolderEditDialog.h"

BookmarkFolderEditDialog::BookmarkFolderEditDialog(DatabaseManager* dbm,
                                                   long long editFOID, long long addParentFOID,
                                                   OutParams* outParams, QWidget *parent)
    : QDialog(parent), ui(new Ui::BookmarkFolderEditDialog), dbm(dbm)
    , canShowTheDialog(false), outParams(outParams), editFOID(editFOID), addParentFOID(addParentFOID)
{
    ui->setupUi(this);

    BookmarkFolderManager::BookmarkFolderData parentfodata;

    //Get File Archives
    QMap<QString, QString> fileArchives;
    QMap<QString, int> fileArchiveIndexes;
    //`canShowTheDialog` a.k.a `bool success;`
    canShowTheDialog = dbm->files.GetUserFileArchivesAndPaths(fileArchives);
    int i = 0;
    foreach (const QString fa, fileArchives.keys())
    {
        ui->cboDefFileArchive->addItem(fa + "     " + fileArchives[fa], fa);
        fileArchiveIndexes[fa] = i++;
    }

    if (editFOID == -1)
    {
        setWindowTitle("Add Folder");

        canShowTheDialog = dbm->bfs.RetrieveBookmarkFolder(addParentFOID, parentfodata);
        if (!canShowTheDialog)
            return;

        ui->cboDefFileArchive->setCurrentIndex(fileArchiveIndexes[parentfodata.DefFileArchive]);
    }
    else
    {
        setWindowTitle("Edit Folder");

        canShowTheDialog = dbm->bfs.RetrieveBookmarkFolder(editFOID, editOriginalFoData);
        if (!canShowTheDialog)
            return;

        canShowTheDialog = dbm->bfs.RetrieveBookmarkFolder(editOriginalFoData.ParentFOID, parentfodata);
        if (!canShowTheDialog)
            return;

        //Show in the UI.
        ui->leName->setText(editOriginalFoData.Name);
        ui->leDesc->setText(editOriginalFoData.Desc);
        ui->cboDefFileArchive->setCurrentIndex(fileArchiveIndexes[editOriginalFoData.DefFileArchive]);
    }

    //Show parent folder path in the UI.
    QString parentFolderPath = parentfodata.Ex_AbsolutePath;
    if (parentFolderPath.isEmpty())
    {
        parentFolderPath = "<i>(None)</i>";
        ui->lblParentFolderValue->setTextFormat(Qt::RichText); //Was Qt::PlainText in UI designer.
    }
    ui->lblParentFolderValue->setText(parentFolderPath);

    //Get sibling namese for the folder to disallow setting its name like them.
    siblingNames = dbm->bfs.GetChildrenNames(parentfodata.FOID);
    if (editFOID != -1)
        siblingNames.removeAll(editOriginalFoData.Name); //Allow saving this item with the same name.

}

BookmarkFolderEditDialog::~BookmarkFolderEditDialog()
{
    delete ui;
}

bool BookmarkFolderEditDialog::canShow()
{
    return canShowTheDialog;
}

bool BookmarkFolderEditDialog::validate()
{
    QString folderName = ui->leName->text().trimmed();
    if (folderName.length() == 0)
    {
        QMessageBox::warning(this, "Validation Error",
                             "Please enter a non-blank name for the folder.");
        return false;
    }

    if (siblingNames.contains(folderName))
    {
        QMessageBox::warning(this, "Validation Error",
                             "There is already a folder with this name in the parent folder.");
        return false;
    }

    return true;
}

void BookmarkFolderEditDialog::accept()
{
    if (!validate())
        return;

    BookmarkFolderManager::BookmarkFolderData fodata;
    fodata.FOID = editFOID;
    fodata.ParentFOID = (editFOID == -1 ? addParentFOID : editOriginalFoData.ParentFOID);
    fodata.Name = ui->leName->text().trimmed();
    fodata.Desc = ui->leDesc->text().trimmed();
    fodata.DefFileArchive = ui->cboDefFileArchive->currentData().toString();

    bool success = dbm->bfs.AddOrEditBookmarkFolder(editFOID, fodata);
    if (success)
    {
        if (outParams != NULL)
            outParams->addedFOID = editFOID;
        QDialog::accept();
    }
}
