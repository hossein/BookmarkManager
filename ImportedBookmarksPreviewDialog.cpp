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
