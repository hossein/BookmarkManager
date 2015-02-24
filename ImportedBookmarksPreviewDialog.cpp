#include "ImportedBookmarksPreviewDialog.h"
#include "ui_ImportedBookmarksPreviewDialog.h"

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

    QDialog::accept();
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

    if (!isFolder)
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
