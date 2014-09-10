#include "BookmarkEditDialog.h"
#include "ui_BookmarkEditDialog.h"

#include "FileManager.h"
#include "FileViewManager.h"
#include "Util.h"

#include <QDebug>
#include <QtGui/QFileDialog>
#include <QtGui/QInputDialog>
#include <QtGui/QMenu>

//[KeepDefaultFile-1]
//Note: On remove, if defBFID was removed we should set editedDefBFID to -1 so that user has to
//      choose it again or it will be automatically choosed or remained -1 if no files are
//      remaining. We achieve this by not keeping a editedDefBFID and instead having a bool field
//      editedFilesList.

BookmarkEditDialog::BookmarkEditDialog(DatabaseManager* dbm, long long editBId,
                                       OutParams* outParams, QWidget *parent) :
    QDialog(parent), ui(new Ui::BookmarkEditDialog), dbm(dbm),
    canShowTheDialog(false), outParams(outParams),
    originalEditBId(editBId), editBId(editBId) //[why-two-editbids]
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

        //[No-File-Model-Yet]
        //Note: We don't retrieve the files model and use custom QList's and QTableWidget instead.
        //canShowTheDialog = dbm->files.RetrieveBookmarkFilesModel(editBId, editOriginalBData.Ex_FilesModel);
        //if (!canShowTheDialog)
        //    return;

        canShowTheDialog = dbm->files.RetrieveBookmarkFiles(editBId, editOriginalBData.Ex_FilesList);
        if (!canShowTheDialog)
            return;

        //Additional bookmark variables.
        editedFilesList = editOriginalBData.Ex_FilesList;
        SetDefaultBFID(editOriginalBData.DefBFID); //Needed; retrieving functions don't set this.

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
    //Choose a default file ourselves if user hasn't made a choice.
    if (editedFilesList.size() > 0 && DefaultFileIndex() == -1)
    {
        QStringList fileNames;
        foreach(const FileManager::BookmarkFile& bf, editedFilesList)
            fileNames.append(bf.OriginalName);

        int editedDefBFIDindex = dbm->fview.ChooseADefaultFileBasedOnExtension(fileNames);
        SetDefaultFileToIndex(editedDefBFIDindex);
        //editedDefBFID = editedFilesList[editedDefBFIDindex].BFID;
    }

    //TODO: If already exists, show warning, switch to the already existent, etc etc
    BookmarkManager::BookmarkData bdata;
    bdata.BID = editBId; //Not important.
    bdata.Name = ui->leName->text().trimmed();
    bdata.URL = ui->leURL->text().trimmed();
    bdata.Desc = ui->ptxDesc->toPlainText();
    bdata.DefBFID = -1; //[KeepDefaultFile-1]
    bdata.Rating = ui->dialRating->value();

    QStringList tagsList = ui->leTags->text().split(' ', QString::SkipEmptyParts);

    bool success;
    QList<long long> associatedTIDs;

    //We do NOT use the transactions in a nested manner. This 'linear' ('together') mode is was
    //  not only and easier to understand, it's better for error management, too.
    //IMPORTANT: In case of RollBack, do NOT return!
    //           [why-two-editbids]: In case of ADDing, MUST set editBId to its original `-1` value!
    //           Thanksfully FilesList don't need changing.
    dbm->db.transaction();
    dbm->files.BeginFilesTransaction();
    {
        success = dbm->bms.AddOrEditBookmark(editBId, bdata); //For Add, the editBID will be modified!
        if (!success)
            return DoRollBackAction();

        success = dbm->tags.SetBookmarkTags(editBId, tagsList, associatedTIDs);
        if (!success)
            return DoRollBackAction();

        QList<long long> updatedBFIDs;
        success = dbm->files.UpdateBookmarkFiles(editBId,
                                                 editOriginalBData.Ex_FilesList, editedFilesList,
                                                 updatedBFIDs);
        if (!success)
            return DoRollBackAction();

        //Set the default file BFID for the bookmark. We already set defBFID to `-1` in the database
        //  according to [KeepDefaultFile-1], so we only do it if it's other than -1.
        //The following line is WRONG and can't be used! Our original `editedFilesList` hasn't changed!
        //long long editedDefBFID = DefaultBFID(editedFilesList);
        long long editedDefBFIDIndex = DefaultFileIndex();
        if (editedDefBFIDIndex != -1)
        {
            long long editedDefBFID = updatedBFIDs[editedDefBFIDIndex];
            success = dbm->bms.SetBookmarkDefBFID(editBId, editedDefBFID);
        }
        if (!success)
            return DoRollBackAction();
    }
    dbm->files.CommitFilesTransaction(); //Committing files transaction doesn't fail!
    dbm->db.commit();

    if (success)
    {
        if (outParams != NULL)
        {
            outParams->addedBId = editBId;
            outParams->associatedTIDs.append(associatedTIDs);
        }

        QDialog::accept();
    }
}

void BookmarkEditDialog::DoRollBackAction()
{
    dbm->db.rollback();
    editBId = originalEditBId; //Affects only the Adding mode.

    bool rollbackResult = dbm->files.RollBackFilesTransaction();
    if (!rollbackResult)
    {
        QMessageBox::critical(this, "File Transaction Rollback Error", "Not all changes made "
                              "to your filesystem in the intermediary process of adding the "
                              "bookmark could not be reverted.<br/><br/>"
                              "<b>Your filesystem may be in inconsistent state!</b>");
    }
}

void BookmarkEditDialog::on_dialRating_valueChanged(int value)
{
    ui->lblRatingValue->setText(QString::number(value / 10.0, 'f', 1));
}

void BookmarkEditDialog::on_dialRating_sliderMoved(int position)
{
    Q_UNUSED(position);
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

void BookmarkEditDialog::SetDefaultBFID(long long BFID)
{
    //`false` values must also be set.
    for (int i = 0; i < editedFilesList.size(); i++)
        editedFilesList[i].Ex_IsDefaultFileForEditedBookmark = (editedFilesList[i].BFID == BFID);
}

void BookmarkEditDialog::SetDefaultFileToIndex(int bfIdx)
{
    //`false` values must also be set.
    for (int i = 0; i < editedFilesList.size(); i++)
        editedFilesList[i].Ex_IsDefaultFileForEditedBookmark = (i == bfIdx);
}

long long BookmarkEditDialog::DefaultBFID()
{
    foreach (const FileManager::BookmarkFile& bf, editedFilesList)
        if (bf.Ex_IsDefaultFileForEditedBookmark)
            return bf.BFID;
    return -1;
}

int BookmarkEditDialog::DefaultFileIndex()
{
    for (int i = 0; i < editedFilesList.size(); i++)
        if (editedFilesList[i].Ex_IsDefaultFileForEditedBookmark)
            return i;
    return -1;
}

void BookmarkEditDialog::on_btnShowAttachUI_clicked()
{
    af_showAttachUI();
}

void BookmarkEditDialog::on_btnSetFileAsDefault_clicked()
{
    af_setAsDefault();
}

void BookmarkEditDialog::on_twAttachedFiles_itemActivated(QTableWidgetItem* item)
{
    Q_UNUSED(item);
    af_previewOrOpen();
}

void BookmarkEditDialog::on_twAttachedFiles_itemSelectionChanged()
{
    ui->btnSetFileAsDefault->setEnabled(!(ui->twAttachedFiles->selectedItems().isEmpty()));
}

void BookmarkEditDialog::on_twAttachedFiles_customContextMenuRequested(const QPoint& pos)
{
    //[Clear selection on useless right-click]
    if (ui->twAttachedFiles->itemAt(pos) == NULL)
        ui->twAttachedFiles->clearSelection();

    //Now  check for selection.
    int filesListIdx;
    if (ui->twAttachedFiles->selectedItems().empty())
        filesListIdx = -1;
    else
        filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    bool fileSelected = (filesListIdx != -1);

    typedef QKeySequence QKS;
    QMenu afMenu("Attached File Menu");

    if (!fileSelected)
    {
        QAction* a_attach = afMenu.addAction("&Attach Files...",this,SLOT(af_showAttachUI()));
        Q_UNUSED(a_attach);
    }
    else
    {
        QAction* a_preview  = afMenu.addAction("&Preview"        , this, SLOT(af_preview()),    QKS("Enter"));
        QAction* a_open     = afMenu.addAction("&Open"           , this, SLOT(af_open()),       QKS("Shift+Enter"));
        QAction* a_edit     = afMenu.addAction("Open (&Editable)", this, SLOT(af_edit()));
        QAction* a_openWith = afMenu.addAction("Open Wit&h..."   , this, SLOT(af_openWith()),   QKS("Ctrl+Enter"));
                              afMenu.addSeparator();
        QAction* a_setDef   = afMenu.addAction("Set &As Default" , this, SLOT(af_setAsDefault()));
        QAction* a_rename   = afMenu.addAction("Rena&me"         , this, SLOT(af_rename()));
        QAction* a_remove   = afMenu.addAction("Remo&ve"         , this, SLOT(af_remove()),     QKS("Del"));
                              afMenu.addSeparator();
        QAction* a_props    = afMenu.addAction("P&roperties"     , this, SLOT(af_properties()), QKS("Alt+Enter"));

        Q_UNUSED(a_edit);
        Q_UNUSED(a_openWith);
        Q_UNUSED(a_remove);
        Q_UNUSED(a_props);

        bool canPreview = dbm->fview.HasPreviewHandler(editedFilesList[filesListIdx].OriginalName);
        a_preview->setEnabled(canPreview);
        afMenu.setDefaultAction(canPreview ? a_preview : a_open);

        a_setDef->setEnabled(!editedFilesList[filesListIdx].Ex_IsDefaultFileForEditedBookmark);
        a_rename->setEnabled(editedFilesList[filesListIdx].BFID != -1);
    }

    QPoint menuPos = ui->twAttachedFiles->viewport()->mapToGlobal(pos);
    afMenu.exec(menuPos);
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
    //Note: Multiple file handling:
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
        //Note: BOTH `BFID` and `FID` are CRUCIAL to be set to `-1`!
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
        bf.Ex_IsDefaultFileForEditedBookmark = false;
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

void BookmarkEditDialog::af_showAttachUI()
{
    ui->stwFileAttachments->setCurrentWidget(ui->pageAttachNew);
}

void BookmarkEditDialog::af_previewOrOpen()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    bool canPreview = dbm->fview.HasPreviewHandler(editedFilesList[filesListIdx].OriginalName);
    if (canPreview)
        af_preview();
    else
        af_open();
}

void BookmarkEditDialog::af_preview()
{
    //TODO: Use a preview handler;
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    //dbm->fview.Preview(GetAttachedFileFullPathName(filesListIdx));
}

void BookmarkEditDialog::af_open()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    dbm->fview.OpenReadOnly(GetAttachedFileFullPathName(filesListIdx));
}

void BookmarkEditDialog::af_edit()
{
    //NOTE: We want to allow editing of yet-unattached files?
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    dbm->fview.OpenEditable(GetAttachedFileFullPathName(filesListIdx));
}

void BookmarkEditDialog::af_openWith()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    dbm->fview.OpenWith(GetAttachedFileFullPathName(filesListIdx));
}

void BookmarkEditDialog::af_setAsDefault()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    SetDefaultFileToIndex(filesListIdx);
    //editedDefBFID = editedFilesList[filesListIdx].BFID;
    PopulateUIFiles(true);
    ui->twAttachedFiles->setFocus();
}

void BookmarkEditDialog::af_rename()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    const QString originalName = editedFilesList[filesListIdx].OriginalName;
    QString currentFileNameOnly = dbm->files.GetFileNameOnlyFromOriginalNameField(originalName);

    bool renameConfirmed;
    QString changedName = QInputDialog::getText(this, "Rename File", "File Name", QLineEdit::Normal,
                                                currentFileNameOnly, &renameConfirmed);
    if (!renameConfirmed)
        return;

    changedName = changedName.trimmed();
    if (!Util::IsValidFileName(changedName))
    {
        QMessageBox::critical(this, "Invalid Name", "Invalid File Name \"" + changedName + "\".\n"
                              "The file wasn't renamed.");
        return;
    }

    //Rename the file.
    editedFilesList[filesListIdx].OriginalName =
            dbm->files.ChangeOriginalNameField(originalName, changedName);
    PopulateUIFiles(true);
}

void BookmarkEditDialog::af_remove()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();



    QString removeConfirmText;
    if (editedFilesList[filesListIdx].BFID == -1)
    {
        removeConfirmText = "Are you sure you do not want to include the file \"" +
                editedFilesList[filesListIdx].OriginalName + "\" with this bookmark?";
    }
    else
    {
        const QString userReadableFileName =
                dbm->files.GetUserReadableArchiveFilePath(editedFilesList[filesListIdx].OriginalName);
        removeConfirmText = "Are you sure you want to remove the file \"" + userReadableFileName +
                "\"?\nIt will be removed from the File Archive as well, if no other bookmarks "
                "are referencing it.";
    }

    int removeConfirmed = (QMessageBox::Yes == QMessageBox::question(
                this, "Remove File", removeConfirmText, QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No));

    if (!removeConfirmed)
        return;

    //Remove the file.
    editedFilesList.removeAt(filesListIdx);
    PopulateUIFiles(true);
}

void BookmarkEditDialog::af_properties()
{
    int filesListIdx = ui->twAttachedFiles->selectedItems()[0]->data(Qt::UserRole).toInt();
    dbm->fview.ShowProperties(GetAttachedFileFullPathName(filesListIdx));
}

QString BookmarkEditDialog::GetAttachedFileFullPathName(int filesListIdx)
{
    QString fullFilePathName;

    if (editedFilesList[filesListIdx].BFID == -1)
        fullFilePathName = editedFilesList[filesListIdx].OriginalName;
    else
        fullFilePathName = dbm->files.GetFullArchiveFilePath(
                           editedFilesList[filesListIdx].ArchiveURL);

    return fullFilePathName;
}
