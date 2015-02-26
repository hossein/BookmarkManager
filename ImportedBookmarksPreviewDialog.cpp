#include "ImportedBookmarksPreviewDialog.h"
#include "ui_ImportedBookmarksPreviewDialog.h"

#include <QDebug>

ImportedBookmarksPreviewDialog::ImportedBookmarksPreviewDialog(
        DatabaseManager* dbm, Config* conf, ImportedEntityList* elist, QWidget *parent)
    : QDialog(parent), ui(new Ui::ImportedBookmarksPreviewDialog), dbm(dbm), conf(conf)
    , canShowTheDialog(false), elist(elist)
{
    ui->setupUi(this);

    ui->leTagsForAll->setModel(&dbm->tags.model);
    ui->leTagsForAll->setModelColumn(dbm->tags.tidx.TagName);
    ui->leTagsForBookmark->setModel(&dbm->tags.model);
    ui->leTagsForBookmark->setModelColumn(dbm->tags.tidx.TagName);
    ui->leTagsForFolder->setModel(&dbm->tags.model);
    ui->leTagsForFolder->setModelColumn(dbm->tags.tidx.TagName);

    ui->grpBookmarkProps->setVisible        (false);
    ui->grpDuplBookmarkProps->setVisible    (false);
    ui->grpDuplBookmarkSameProps->setVisible(false);
    ui->grpBookmarkFolderProps->setVisible  (false);

    connect(ui->chkImportBookmark, SIGNAL(toggled(bool)), ui->leTagsForBookmark, SLOT(setEnabled(bool)));
    connect(ui->chkImportFolder  , SIGNAL(toggled(bool)), ui->leTagsForFolder  , SLOT(setEnabled(bool)));

    AddItems();

    canShowTheDialog = true;
}

ImportedBookmarksPreviewDialog::~ImportedBookmarksPreviewDialog()
{
    delete ui;
}

bool ImportedBookmarksPreviewDialog::canShow()
{
    return canShowTheDialog;
}

void ImportedBookmarksPreviewDialog::accept()
{
    //Check if there are any undecided similar bookmarks to be imported.
    foreach (const ImportedBookmark& ib, elist->iblist)
    {
        if (ib.Ex_import && ib.Ex_status == ImportedBookmark::S_AnalyzedSimilarExistent)
        {
            QTreeWidgetItem* twi = bookmarkItems[ib.intId];
            ui->twBookmarks->setCurrentItem(twi);
            ui->twBookmarks->scrollToItem(twi);

            QMessageBox::information(this, "Items Need Review", "One or more of the bookmarks for import are similar to an existing "
                                                                "bookmark. You must decide what to do with the new bookmark first.");
            return;
        }
    }


    //TOOD: add tags of folder and ALL to bms.
    //      don't import bms that parent folder is not gonna be importd.

    QDialog::accept();
}

void ImportedBookmarksPreviewDialog::on_twBookmarks_itemSelectionChanged()
{
    if (ui->twBookmarks->selectedItems().isEmpty())
        return;

    bool isFolder = ui->twBookmarks->selectedItems()[0]->data(0, TWID_IsFolder).toBool();
    int index = ui->twBookmarks->selectedItems()[0]->data(0, TWID_Index).toInt();

    ui->grpBookmarkFolderProps->setVisible  ( isFolder);
    ui->grpBookmarkProps->setVisible        (!isFolder);
    ui->grpDuplBookmarkProps->setVisible    (false);
    ui->grpDuplBookmarkSameProps->setVisible(false);

    if (isFolder)
    {
        ui->chkImportFolder->setChecked(elist->ibflist[index].Ex_importBookmarks);
        ui->leTagsForFolder->setText(elist->ibflist[index].Ex_additionalTags.join(' '));
    }
    else //if (!isFolder), is a bookmark
    {
        const ImportedBookmark::ImportedBookmarkStatus status = elist->iblist[index].Ex_status;
        ui->chkImportBookmark->setChecked(elist->iblist[index].Ex_import);
        ui->leTagsForBookmark->setText(elist->iblist[index].Ex_additionalTags.join(' ')); //TODO: Monitor user changes to this text box and other elements.
        //TODO: All statuses are possible, even user-chosen ones.
        //TODO: need to check user selection for duplicate, non-same bookmarks.
        if (status == ImportedBookmark::S_AnalyzedExactExistent ||
            status == ImportedBookmark::S_ReplaceExisting ||
            status == ImportedBookmark::S_AppendToExisting)
        {
            ui->chkImportBookmark->setVisible(false);
            ui->grpDuplBookmarkProps->setVisible(true);

            if (elist->iblist[index].Ex_import)
            {
                ui->optKeep->setChecked(false);
                ui->optOverwrite->setChecked(status == ImportedBookmark::S_ReplaceExisting);
                ui->optAppend->setChecked(status == ImportedBookmark::S_AppendToExisting);
            }
            else
            {
                ui->optKeep->setChecked(true);
                ui->optOverwrite->setChecked(false);
                ui->optAppend->setChecked(false);
            }
        }
        else if (status == ImportedBookmark::S_AnalyzedSimilarExistent)
        {
            ui->chkImportBookmark->setVisible(true);
            ui->grpDuplBookmarkSameProps->setVisible(true);
        }
        else if (status == ImportedBookmark::S_AnalyzedImportOK)
        {
            ui->chkImportBookmark->setVisible(true);
        }
    }
}

void ImportedBookmarksPreviewDialog::on_leTagsForAll_editingFinished()
{
    //TODO
}

void ImportedBookmarksPreviewDialog::on_chkImportBookmark_clicked()
{
    //TODO: In all of the following change icons as well.

    QTreeWidgetItem* twi = ui->twBookmarks->selectedItems()[0];
    int index = twi->data(0, TWID_Index).toInt();
    elist->iblist[index].Ex_import = ui->chkImportBookmark->isChecked();
}

void ImportedBookmarksPreviewDialog::on_leTagsForBookmark_editingFinished()
{
    QTreeWidgetItem* twi = ui->twBookmarks->selectedItems()[0];
    int index = twi->data(0, TWID_Index).toInt();
    QStringList tagsList = ui->leTagsForBookmark->text().split(' ', QString::SkipEmptyParts);
    elist->iblist[index].Ex_additionalTags = tagsList;
}

void ImportedBookmarksPreviewDialog::on_optKeep_clicked()
{
    if (!ui->optKeep->isChecked())
        return;

    QTreeWidgetItem* twi = ui->twBookmarks->selectedItems()[0];
    int index = twi->data(0, TWID_Index).toInt();
    elist->iblist[index].Ex_import = false;
}

void ImportedBookmarksPreviewDialog::on_optOverwrite_clicked()
{
    if (!ui->optOverwrite->isChecked())
        return;

    QTreeWidgetItem* twi = ui->twBookmarks->selectedItems()[0];
    int index = twi->data(0, TWID_Index).toInt();
    elist->iblist[index].Ex_import = true;
    elist->iblist[index].Ex_status = ImportedBookmark::S_ReplaceExisting;
}

void ImportedBookmarksPreviewDialog::on_optAppend_clicked()
{
    if (!ui->optAppend->isChecked())
        return;

    QTreeWidgetItem* twi = ui->twBookmarks->selectedItems()[0];
    int index = twi->data(0, TWID_Index).toInt();
    elist->iblist[index].Ex_import = true;
    elist->iblist[index].Ex_status = ImportedBookmark::S_AppendToExisting;
}

void ImportedBookmarksPreviewDialog::on_chkImportFolder_clicked()
{
    bool import = ui->chkImportFolder->isChecked();
    QTreeWidgetItem* twi = ui->twBookmarks->selectedItems()[0];
    RecursiveSetFolderImport(twi, import);
}

void ImportedBookmarksPreviewDialog::on_leTagsForFolder_editingFinished()
{
    QTreeWidgetItem* twi = ui->twBookmarks->selectedItems()[0];
    int index = twi->data(0, TWID_Index).toInt();
    QStringList tagsList = ui->leTagsForFolder->text().split(' ', QString::SkipEmptyParts);
    elist->ibflist[index].Ex_additionalTags = tagsList;
}

void ImportedBookmarksPreviewDialog::AddItems()
{
    QIcon icon_folder (":/res/import_folder.png");
    QIcon icon_okay   (":/res/import_ok.png");
    QIcon icon_similar(":/res/import_duplicate_similar.png");
    QIcon icon_exact  (":/res/import_duplicate_exact.png");

    int index = 0;
    foreach (const ImportedBookmarkFolder& ibf, elist->ibflist)
    {
        QTreeWidgetItem* twi = new QTreeWidgetItem();
        twi->setText(0, "[" + QString::number(ibf.intId) + "] " + ibf.title + " [" + ibf.root + "]");
        twi->setIcon(0, icon_folder);
        twi->setData(0, TWID_IsFolder, true);
        twi->setData(0, TWID_Index, index);
        folderItems[ibf.intId] = twi;

        if (ibf.parentId <= 0)
            ui->twBookmarks->addTopLevelItem(twi);
        else
            folderItems[ibf.parentId]->addChild(twi);

        index++;
    }

    index = 0;
    foreach (const ImportedBookmark& ib, elist->iblist)
    {
        QTreeWidgetItem* twi = new QTreeWidgetItem();
        twi->setText(0, ib.title + " [" + QString::number(index) + "]");
        twi->setToolTip(0, ib.uri);
        twi->setData(0, TWID_IsFolder, false);
        twi->setData(0, TWID_Index, index);
        bookmarkItems[ib.intId] = twi;

        if (ib.Ex_status == ImportedBookmark::S_AnalyzedExactExistent)
            twi->setIcon(0, icon_exact);
        else if (ib.Ex_status == ImportedBookmark::S_AnalyzedSimilarExistent)
            twi->setIcon(0, icon_similar);
        else if (ib.Ex_status == ImportedBookmark::S_AnalyzedImportOK)
            twi->setIcon(0, icon_okay);

        if (ib.parentId <= 0)
            ui->twBookmarks->addTopLevelItem(twi);
        else
            folderItems[ib.parentId]->addChild(twi);

        index++;
    }

    ui->twBookmarks->expandAll();
    ui->twBookmarks->setCurrentItem(ui->twBookmarks->topLevelItem(0));
}

void ImportedBookmarksPreviewDialog::RecursiveSetFolderImport(QTreeWidgetItem* twi, bool import)
{
    int index = twi->data(0, TWID_Index).toInt();
    int folderId = elist->ibflist[index].intId;

    QIcon icon_folder(":/res/import_folder.png");
    QIcon icon_folderdontimport(":/res/import_folderdontimport.png");

    QIcon icon_dontimport(":/res/import_dontimport.png");
    QIcon icon_okay      (":/res/import_ok.png");
    QIcon icon_similar   (":/res/import_duplicate_similar.png");
    QIcon icon_exact     (":/res/import_duplicate_exact.png");
    QIcon icon_overwrite (":/res/import_overwrite.png");
    QIcon icon_append    (":/res/import_append.png");

    //Update import status and icon of the node.
    elist->ibflist[index].Ex_importBookmarks = import;
    folderItems[folderId]->setIcon(0, import ? icon_folder : icon_folderdontimport);

    //Update icons of all subbookmarks.
    QTreeWidgetItem* bmtwi;
    for (int i = 0; i < elist->iblist.size(); i++)
    {
        ImportedBookmark& ib = elist->iblist[i];
        if (ib.parentId == folderId)
        {
            qDebug() << "Set that";
            ib.Ex_import = import;
            bmtwi = bookmarkItems[ib.intId];
            bmtwi->setDisabled(!import);
            if (!import)
            {
                bmtwi->setIcon(0, icon_dontimport);
            }
            else
            {
                if (ib.Ex_import == false)
                    bmtwi->setIcon(0, icon_dontimport);
                else if (ib.Ex_status == ImportedBookmark::S_AnalyzedExactExistent)
                    bmtwi->setIcon(0, icon_exact);
                else if (ib.Ex_status == ImportedBookmark::S_AnalyzedSimilarExistent)
                    bmtwi->setIcon(0, icon_similar);
                else if (ib.Ex_status == ImportedBookmark::S_AnalyzedImportOK)
                    bmtwi->setIcon(0, icon_okay);
                else if (ib.Ex_status == ImportedBookmark::S_ReplaceExisting)
                    bmtwi->setIcon(0, icon_overwrite);
                else if (ib.Ex_status == ImportedBookmark::S_AppendToExisting)
                    bmtwi->setIcon(0, icon_append);
            }
        }
    }

    //Recursively update sub-folders
    QTreeWidgetItem* childdirtwi;
    for (int i = 0; i < elist->ibflist.size(); i++)
    {
        ImportedBookmarkFolder& ibf = elist->ibflist[i];
        if (ibf.parentId == folderId)
        {
            qDebug() << "Set that folder";
            ibf.Ex_importBookmarks = import;
            childdirtwi = folderItems[ibf.intId];
            childdirtwi->setDisabled(!import); //Set disabled as well as setting icon and handling children.
            RecursiveSetFolderImport(childdirtwi, import);
        }
    }
}
