#include "ImportedBookmarksPreviewDialog.h"
#include "ui_ImportedBookmarksPreviewDialog.h"

#include "Config.h"
#include "ImportedBookmarksProcessor.h"
#include "Util.h"

#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QUrl>

ImportedBookmarksPreviewDialog::ImportedBookmarksPreviewDialog(DatabaseManager* dbm, BookmarkImporter* bmim,
                                                               ImportedEntityList* elist, QWidget *parent)
    : QDialog(parent), ui(new Ui::ImportedBookmarksPreviewDialog), dbm(dbm)
    , canShowTheDialog(false), elist(elist)
{
    ui->setupUi(this);

    setWindowTitle(QString(windowTitle() + " (%1 bookmarks)").arg(elist->iblist.size()));

    //With only stretches applied to the layout, the left widget changed size on each bookmark select
    //  with different types (e.g exactly similar, already similar etc). We need to fix the size.
    int sizeForDPI = 300 * (qApp->screens()[0]->logicalDotsPerInch() / 96.0);
    ui->widLeftPane->setFixedWidth(sizeForDPI);

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

    m_bookmarksProcessor = new ImportedBookmarksProcessor(dbm->conf->concurrentBookmarkProcessings, bmim, this, this);
    connect(m_bookmarksProcessor, SIGNAL(ProcessingDone()), this, SLOT(ProcessingDone()));
    connect(m_bookmarksProcessor, SIGNAL(ProcessingCanceled()), this, SLOT(ProcessingCanceled()));
    connect(m_bookmarksProcessor, SIGNAL(ImportedBookmarkProcessed(ImportedBookmark*,bool)),
            this, SLOT(ImportedBookmarkProcessed(ImportedBookmark*,bool)));

    icon_folder           = QIcon(":/res/import_folder.png");
    icon_folderdontimport = QIcon(":/res/import_folderdontimport.png");
    icon_dontimport       = QIcon(":/res/import_dontimport.png");
    icon_okay             = QIcon(":/res/import_ok.png");
    icon_similar          = QIcon(":/res/import_duplicate_similar.png");
    icon_exact            = QIcon(":/res/import_duplicate_exact.png");
    icon_overwrite        = QIcon(":/res/import_overwrite.png");
    icon_append           = QIcon(":/res/import_append.png");
    icon_waiting          = QIcon(":/res/import_waiting.png");
    icon_fail             = QIcon(":/res/import_fail.png");

    //Initialize the data we will need during the conversion.
    for (int i = 0; i < elist->ibflist.size(); i++)
        folderItemsIndexInArray[elist->ibflist[i].intId] = i;

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
    //1. Add tags that user has specified for folders and also all of the bookmarks to each bookmark.
    //   Note: The functions that set bookmark tags handle duplicate tags so we don't check for
    //   duplicate tags here at all.

    //1a. Begin with applying the tags of parent folders to their child folders recursively.
    //  (Use `Ex_semiFinalTags` instead of `Ex_additionalTags` to avoid changing folders'
    //   user-settable properties in case user cancels the confirm message or processing dialog).
    for (int folderidx = 0; folderidx < elist->ibflist.size(); folderidx++)
        elist->ibflist[folderidx].Ex_semiFinalTags = elist->ibflist[folderidx].Ex_additionalTags;
    for (int parentidx = 0; parentidx < elist->ibflist.size(); parentidx++)
    {
        elist->ibflist[parentidx].Ex_finalTags = elist->ibflist[parentidx].Ex_semiFinalTags;

        for (int childidx = 0; childidx < elist->ibflist.size(); childidx++)
            if (elist->ibflist[childidx].parentId == elist->ibflist[parentidx].intId)
                elist->ibflist[childidx].Ex_semiFinalTags.append(elist->ibflist[parentidx].Ex_finalTags);
    }

    //1b. Add the tagsForAll tags; we didn't add these in the previous `for` to prevent them being
    //  added from parents to children again and again.
    for (int parentidx = 0; parentidx < elist->ibflist.size(); parentidx++)
        elist->ibflist[parentidx].Ex_finalTags.append(tagsForAll);

    //1c. Now simply copy the tags from each folder for its children.
    //2. Also, all the folders that are not going to be imported are already specified RECURSIVELY
    //   when the user clicked the checkboxes, so we also just set the import value for bookmarks
    //   from their parent folders.
    int parentFolderIndex;
    int importedCount = 0;
    for (int i = 0; i < elist->iblist.size(); i++)
    {
        elist->iblist[i].Ex_finalTags = elist->iblist[i].Ex_additionalTags;

        parentFolderIndex = folderItemsIndexInArray[elist->iblist[i].parentId];
        elist->iblist[i].Ex_finalTags.append(elist->ibflist[parentFolderIndex].Ex_finalTags);

        bool doImport = (elist->ibflist[parentFolderIndex].Ex_importBookmarks
                         ? elist->iblist[i].Ex_import
                         : false);
        elist->iblist[i].Ex_finalImport = doImport;
        importedCount += (doImport ? 1 : 0);
    }

    //AFTER all bookmarks have derived the import status of their parent folders:
    //Check if there are any undecided similar bookmarks to be imported.
    //Also collect some stats.
    int similarDuplicateBookmarksToBeImported = 0;
    int exactDuplicateBookmarksToBeIgnored = 0;
    for (int i = 0; i < elist->iblist.size(); i++)
    {
        //By reference
        ImportedBookmark& ib = elist->iblist[i];

        if (!ib.Ex_finalImport) //Not Ex_import
            continue;

        if (ib.Ex_status == ImportedBookmark::S_AnalyzedSimilarExistent)
        {
            QTreeWidgetItem* twi = bookmarkItems[ib.intId];
            ui->twBookmarks->setCurrentItem(twi);
            ui->twBookmarks->scrollToItem(twi);

            QMessageBox::information(this, "Items Need Review", "One or more of the bookmarks for import are similar to an existing "
                                                                "bookmark. You must decide what to do with the new bookmark first.");
            return;
        }
        else if (ib.Ex_status == ImportedBookmark::S_ReplaceExisting ||
                 ib.Ex_status == ImportedBookmark::S_AppendToExisting)
        {
            similarDuplicateBookmarksToBeImported += 1;
        }
        else if (ib.Ex_status == ImportedBookmark::S_AnalyzedExactExistent)
        {
            ib.Ex_finalImport = false;
            exactDuplicateBookmarksToBeIgnored += 1;
            importedCount -= 1;
        }
    }

    //Show import statistics to user and confirm.
    if (importedCount == 0)
    {
        QMessageBox::information(this, "Nothing To Import", "No bookmark is selected for import! Select bookmarks or folders first.");
        return;
    }
    else
    {
        QString message = QString("%1 bookmark(s) are going to be imported, %2 bookmarks will not be imported.")
                          .arg(importedCount).arg(elist->iblist.size() - importedCount);
        if (similarDuplicateBookmarksToBeImported > 0)
            message += QString("\n%1 bookmark(s) are similar to bookmarks already in database and will be merged or will replace them.")
                       .arg(similarDuplicateBookmarksToBeImported);
        if (exactDuplicateBookmarksToBeIgnored > 0)
            message += QString("\nAlso %1 bookmark(s) exactly look like bookmarks already in database and will be ignored.")
                       .arg(exactDuplicateBookmarksToBeIgnored);
        message += "\n\nContinue?";
        int result = QMessageBox::information(this, "Bookmarks To Import", message, QMessageBox::Yes | QMessageBox::No);
        if (result != QMessageBox::Yes)
            return;
    }

    //Disable UI
    ui->widLeftPane->setVisible(false);
    ui->buttonBox->setEnabled(false);

    //Change icons of ImportOK bookmarks from a big green check mark to a 'loading file' icon.
    //  This is to make the item less shiny! than the 'Imported' icon (which is the Exact Duplicate
    //  icon) during importing.
    foreach (const ImportedBookmark& ib, elist->iblist)
    {
        if (ib.Ex_finalImport == true && ib.Ex_status == ImportedBookmark::S_AnalyzedImportOK)
            bookmarkItems[ib.intId]->setIcon(0, icon_waiting);
    }

    //Begin processing
    m_bookmarksProcessor->BeginProcessing(elist);
    //Now `ProcessingDone` or `ProcessingCancelled` signal/slot connections will finish the job,
    //accepting the dialog if necessary.
}

void ImportedBookmarksPreviewDialog::ProcessingDone()
{
    QDialog::accept();
}

void ImportedBookmarksPreviewDialog::ProcessingCanceled()
{
    ///Let the UI work again
    //ui->widLeftPane->setVisible(true);
    //ui->buttonBox->setEnabled(true);

    ///Instead of letting the UI work again, we CLOSE the dialog if user cancels the progress dlg.
    ///  Otherwise we have to recalculate all statuses and show a 'Close' button.

    //Rejecting here merely means user stopped the import operation in the middle.
    QDialog::reject();
}

void ImportedBookmarksPreviewDialog::ImportedBookmarkProcessed(ImportedBookmark* ib, bool successful)
{
    //Must not happen. Just in case.
    if (ib == NULL || !bookmarkItems.contains(ib->intId))
        return;

    //Set icon as Done or Fail. Also correct the title if it was a [title-less bookmarks].
    //  Correcting its text formatting is difficult and potentially removes information that user
    //  previously saw, so we don't do it.
    QTreeWidgetItem* twi = bookmarkItems[ib->intId];
    if (successful)
    {
        twi->setIcon(0, icon_okay);
        twi->setText(0, ib->title);
    }
    else
    {
        twi->setIcon(0, icon_fail);
    }
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
        ui->leTagsForBookmark->setText(elist->iblist[index].Ex_additionalTags.join(' '));

        if (status == ImportedBookmark::S_AnalyzedSimilarExistent ||
            status == ImportedBookmark::S_ReplaceExisting ||
            status == ImportedBookmark::S_AppendToExisting)
        {
            ui->chkImportBookmark->setVisible(false);
            ui->grpDuplBookmarkProps->setVisible(true);

            if (elist->iblist[index].Ex_import)
            {
                if (status == ImportedBookmark::S_AnalyzedSimilarExistent)
                {
                    //Just calling `setChecked(false)` on all of them does not work!
                    //http://stackoverflow.com/questions/9372992/qradiobutton-check-uncheck-issue-in-qt
                    QList<QRadioButton*> rbs = ui->grpDuplBookmarkProps->findChildren<QRadioButton*>();
                    for (int i = 0; i < rbs.size(); i++)
                    {
                        rbs[i]->setAutoExclusive(false);
                        rbs[i]->setChecked(false);
                        rbs[i]->setAutoExclusive(true);
                    }
                }
                else
                {
                    ui->optKeep->setChecked(false);
                    ui->optOverwrite->setChecked(status == ImportedBookmark::S_ReplaceExisting);
                    ui->optAppend->setChecked(status == ImportedBookmark::S_AppendToExisting);
                }
            }
            else
            {
                ui->optKeep->setChecked(true);
                ui->optOverwrite->setChecked(false);
                ui->optAppend->setChecked(false);
            }
        }
        else if (status == ImportedBookmark::S_AnalyzedExactExistent)
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
    tagsForAll = ui->leTagsForAll->text().split(' ', QString::SkipEmptyParts);
}

void ImportedBookmarksPreviewDialog::on_chkImportBookmark_clicked()
{
    bool import = ui->chkImportBookmark->isChecked();
    QTreeWidgetItem* twi = ui->twBookmarks->selectedItems()[0];
    int index = twi->data(0, TWID_Index).toInt();
    elist->iblist[index].Ex_import = import;
    SetBookmarkItemIcon(twi, elist->iblist[index]);
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
    SetBookmarkItemIcon(twi, elist->iblist[index]);
}

void ImportedBookmarksPreviewDialog::on_optOverwrite_clicked()
{
    if (!ui->optOverwrite->isChecked())
        return;

    QTreeWidgetItem* twi = ui->twBookmarks->selectedItems()[0];
    int index = twi->data(0, TWID_Index).toInt();
    elist->iblist[index].Ex_import = true;
    elist->iblist[index].Ex_status = ImportedBookmark::S_ReplaceExisting;
    SetBookmarkItemIcon(twi, elist->iblist[index]);
}

void ImportedBookmarksPreviewDialog::on_optAppend_clicked()
{
    if (!ui->optAppend->isChecked())
        return;

    QTreeWidgetItem* twi = ui->twBookmarks->selectedItems()[0];
    int index = twi->data(0, TWID_Index).toInt();
    elist->iblist[index].Ex_import = true;
    elist->iblist[index].Ex_status = ImportedBookmark::S_AppendToExisting;
    SetBookmarkItemIcon(twi, elist->iblist[index]);
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
    const int rootFolderIntId = (elist->importSource == ImportedEntityList::Source_Firefox ? 1 : 0);

    int index = 0;
    foreach (const ImportedBookmarkFolder& ibf, elist->ibflist)
    {
        QTreeWidgetItem* twi = new QTreeWidgetItem();
        twi->setText(0, ibf.title);
        twi->setIcon(0, icon_folder);
        twi->setData(0, TWID_IsFolder, true);
        twi->setData(0, TWID_Index, index);
        folderItems[ibf.intId] = twi;
        index++;

        //On firefox, don't add or show the root folder which has the ID 1.
        if (ibf.intId == rootFolderIntId && elist->importSource == ImportedEntityList::Source_Firefox)
            continue;

        if (ibf.parentId <= rootFolderIntId)
            ui->twBookmarks->addTopLevelItem(twi);
        else
            folderItems[ibf.parentId]->addChild(twi);
    }

    index = 0;
    foreach (const ImportedBookmark& ib, elist->iblist)
    {
        QTreeWidgetItem* twi = new QTreeWidgetItem();
        twi->setText(0, ib.title);
        SetBookmarkItemIcon(twi, ib);
        twi->setToolTip(0, ib.uri);
        twi->setData(0, TWID_IsFolder, false);
        twi->setData(0, TWID_Index, index);
        bookmarkItems[ib.intId] = twi;
        index++;

        if (ib.title.trimmed().isEmpty())
        {
            //For [title-less bookmarks] show their url in a different formatting.
            twi->setText(0, Util::FullyPercentDecodedUrl(ib.uri));
            twi->setTextColor(0, QColor(192, 128, 0));
            QFont italicFont = twi->font(0);
            italicFont.setItalic(true);
            twi->setFont(0, italicFont);
        }

        if (ib.parentId <= 0)
            ui->twBookmarks->addTopLevelItem(twi);
        else
            folderItems[ib.parentId]->addChild(twi);
    }

    ui->twBookmarks->expandAll();
    ui->twBookmarks->setCurrentItem(ui->twBookmarks->topLevelItem(0));
}

void ImportedBookmarksPreviewDialog::SetBookmarkItemIcon(QTreeWidgetItem* twi, const ImportedBookmark& ib)
{
    if (ib.Ex_import == false)
        twi->setIcon(0, icon_dontimport);
    else if (ib.Ex_status == ImportedBookmark::S_AnalyzedExactExistent)
        twi->setIcon(0, icon_exact);
    else if (ib.Ex_status == ImportedBookmark::S_AnalyzedSimilarExistent)
        twi->setIcon(0, icon_similar);
    else if (ib.Ex_status == ImportedBookmark::S_AnalyzedImportOK)
        twi->setIcon(0, icon_okay);
    else if (ib.Ex_status == ImportedBookmark::S_ReplaceExisting)
        twi->setIcon(0, icon_overwrite);
    else if (ib.Ex_status == ImportedBookmark::S_AppendToExisting)
        twi->setIcon(0, icon_append);
}

void ImportedBookmarksPreviewDialog::RecursiveSetFolderImport(QTreeWidgetItem* twi, bool import)
{
    int index = twi->data(0, TWID_Index).toInt();
    int folderId = elist->ibflist[index].intId;

    //Update import status and icon of the node.
    //IMPORTANT: We set the `import` RECURSIVELY on all sub-folders, and `accept` processing
    //           RELIES ON THIS.
    elist->ibflist[index].Ex_importBookmarks = import;
    folderItems[folderId]->setIcon(0, import ? icon_folder : icon_folderdontimport);

    //Update icons of all subbookmarks.
    QTreeWidgetItem* bmtwi;
    for (int i = 0; i < elist->iblist.size(); i++)
    {
        ImportedBookmark& ib = elist->iblist[i];
        if (ib.parentId == folderId)
        {
            //IMPORTANT: Do NOT set the import on this bookmark so as to prevent clearing user choices.
            //  The final not-importing, etc will be managed by `accept`s processings.
            //ib.Ex_import = import;
            bmtwi = bookmarkItems[ib.intId];
            bmtwi->setDisabled(!import);
            if (!import)
                bmtwi->setIcon(0, icon_dontimport);
            else
                SetBookmarkItemIcon(bmtwi, ib);
        }
    }

    //Recursively update sub-folders
    QTreeWidgetItem* childdirtwi;
    for (int i = 0; i < elist->ibflist.size(); i++)
    {
        ImportedBookmarkFolder& ibf = elist->ibflist[i];
        if (ibf.parentId == folderId)
        {
            ibf.Ex_importBookmarks = import;
            childdirtwi = folderItems[ibf.intId];
            childdirtwi->setDisabled(!import); //Set disabled as well as setting icon and handling children.
            RecursiveSetFolderImport(childdirtwi, import);
        }
    }
}
