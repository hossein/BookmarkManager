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
    QDialog::accept();
}

void ImportedBookmarksPreviewDialog::AddItems()
{
    QIcon icon_folder (":/res/import_folder.png");
    QIcon icon_okay   (":/res/import_ok.png");
    QIcon icon_similar(":/res/import_duplicate_similar.png");
    QIcon icon_exact  (":/res/import_duplicate_exact.png");

    foreach (const ImportedBookmarkFolder& ibf, elist->ibflist)
    {
        QTreeWidgetItem* twi = new QTreeWidgetItem();
        twi->setText(0, ibf.title);
        twi->setIcon(0, icon_folder);
        folderItems[ibf.intId] = twi;

        if (ibf.parentId <= 0)
            ui->twBookmarks->addTopLevelItem(twi);
        else
            folderItems[ibf.parentId]->addChild(twi);
    }

    foreach (const ImportedBookmark& ib, elist->iblist)
    {
        QTreeWidgetItem* twi = new QTreeWidgetItem();
        twi->setText(0, ib.title);
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
    }

    ui->twBookmarks->expandAll();
}
