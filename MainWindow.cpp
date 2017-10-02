#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Bookmarks/BookmarkFilter.h"
#include "Bookmarks/BookmarkEditDialog.h"
#include "Bookmarks/BookmarkViewDialog.h"
#include "Bookmarks/MergeConfirmationDialog.h"
#include "BookmarksBusinessLogic.h"

#include "BookmarkImporter/BookmarkImporter.h"
#include "BookmarkImporter/ImportedBookmarksPreviewDialog.h"
#include "BookmarkImporter/FirefoxBookmarkJSONFileParser.h"
#include "BookmarkImporter/MHTSaver.h"

#include "Settings/SettingsDialog.h"
#include "Util/WindowSizeMemory.h"

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStandardPaths>
#include <QToolButton>
#include <QtSql/QSqlRecord>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), conf(), dbm(this, &conf), m_shouldExit(false)
{
    ui->setupUi(this);

    // Load database
    QString databaseFilePath = QDir::currentPath() + "/" + conf.programDatabasetFileName;
    if (!dbm.BackupOpenOrCreate(databaseFilePath))
    {
        m_shouldExit = true;
        return;
    }

    //Can't do full `RefreshUIDataDisplay` because bv and tf are not initialized yet; this line is
    //  needed before initializing them:
    dbm.PopulateModelsAndInternalTables();

    //After dbm initializing, set up main UI and sizes
    InitializeUIControlsAndPositions();

    // Initialize important controls
    ui->bv->Initialize(&dbm, BookmarksView::LM_FullInformationAndEdit, &dbm.bms.model);
    ui->tf->Initialize(&dbm);
    ui->tv->Initialize(&dbm);

    //After initializing controls and before connecting signals, show data and status in the UI.
    RefreshUIDataDisplay(false /* Already done above */, RA_Focus);

    //Signal connections.
    connect(ui->bv, SIGNAL(activated(long long)), this, SLOT(bvActivated(long long)));
    connect(ui->bv, SIGNAL(selectionChanged(QList<long long>)),
            this, SLOT(bvSelectionChanged(QList<long long>)));
    //Connecting signal after Initialize()ing tf will miss the first emitted signal, which is folder
    //  '0, Unsorted' being activated. But tf is already initialized and has already returned  the
    //  correct FOID=0 in the above `RefreshUIDataDisplay` call thus correct bookmark are shown.
    connect(ui->tf, SIGNAL(CurrentFolderChanged(long long)),
            this,   SLOT(tfCurrentFolderChanged(long long)));
    connect(ui->tf, SIGNAL(RequestMoveBookmarksToFolder(QList<long long>,long long)),
            this,   SLOT(tfRequestMoveBookmarksToFolder(QList<long long>,long long)));
    //The first time no tags are checked, everything is okay; we don't care about missing signals.
    connect(ui->tv, SIGNAL(tagSelectionChanged()), this,  SLOT(tvTagSelectionChanged()));
    //Search area
    connect(ui->leSearch, SIGNAL(textChanged(QString)), this, SLOT(leSearchTextChanged(QString)));
    connect(ui->chkSearchRegExp, SIGNAL(toggled(bool)), this, SLOT(chkSearchRegExpToggled(bool)));

    // Additional sub-parts initialization.
    if (!dbm.files.InitializeFileArchives(&dbm))
    {
        m_shouldExit = true;
        return;
    }
    //The following is not a big deal, we overlook its fails and don't check its return value.
    dbm.files.ClearSandBox();

    qApp->postEvent(this, new QResizeEvent(this->size(), this->size()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::shouldExit() const
{
    return m_shouldExit;
}

void MainWindow::on_btnNew_clicked()
{
    NewBookmark();
}

void MainWindow::on_btnMerge_clicked()
{
    MergeSelectedBookmarks();
}

void MainWindow::on_btnView_clicked()
{
    ViewSelectedBookmark();
}

void MainWindow::on_btnEdit_clicked()
{
    EditSelectedBookmark();
}

void MainWindow::on_btnDelete_clicked()
{
    DeleteSelectedBookmarks();
}

void MainWindow::bvActivated(long long BID)
{
    Q_UNUSED(BID);
    ViewSelectedBookmark();
}

void MainWindow::bvSelectionChanged(const QList<long long>& selectedBIDs)
{
    //Interestingly, selectionChanged signal is not emitted, and thus this function is not called,
    //when moving bookmarks accross folders, because we only hide/show items but the selection is
    //not changed! This is okay as long as the only thing we do with the selection is setting the
    //buttons' enabled state and the like. Initial selection of bookmarks sets the buttons' correct
    //enabled state, which remains valid even after bookmarks are moved.

    bool hasSelection = (!selectedBIDs.empty());
    bool singleSelected = (selectedBIDs.length() == 1);
    bool multiSelected = (selectedBIDs.length() > 1);

    ui->btnMerge->setEnabled(multiSelected);
    ui->btnView->setEnabled(singleSelected);
    ui->btnEdit->setEnabled(singleSelected);
    ui->btnDelete->setEnabled(hasSelection);
}

void MainWindow::tfCurrentFolderChanged(long long FOID)
{
    Q_UNUSED(FOID);
    RefreshUIDataDisplay(false, RA_Focus, QList<long long>(),
                         (UIDDRefreshAction)(RA_SaveSelAndScroll | RA_NoRefreshView));
}

void MainWindow::tfRequestMoveBookmarksToFolder(const QList<long long>& BIDs, long long FOID)
{
    BookmarksBusinessLogic bbLogic(&dbm, this);
    bool success = bbLogic.MoveBookmarksToFolderTrans(BIDs, FOID);
    if (!success)
        return;
    ui->tf->SetCurrentFOIDSilently(FOID);
    RefreshUIDataDisplay(false, RA_CustomSelectAndFocus, BIDs);
}

void MainWindow::tvTagSelectionChanged()
{
    //Now filter the bookmarks according to those tags.
    //Note: Instead of just `bv->SetFilter(...); bv->RefreshView();` we use the full Refresh below.
    //  It's more standard for this task; we want to save selection, and also update status labels.
    //  We don't populate the data again though.
    //20141009: A new save selection method now saves selection upon filtering by tags.
    //  Read the comments in the following function at the place where selection is restored.
    RefreshUIDataDisplay(false, RA_SaveSelAndFocus, QList<long long>(),
                         (UIDDRefreshAction)(RA_SaveSelAndScroll | RA_NoRefreshView));
}

void MainWindow::leSearchTextChanged(const QString& text)
{
    Q_UNUSED(text);
    RefreshUIDataDisplay(false, RA_SaveSel, QList<long long>(), //Do not use RA_Focus here, user continues to type
                         (UIDDRefreshAction)(RA_SaveSelAndScroll | RA_NoRefreshView));
}

void MainWindow::chkSearchRegExpToggled(bool checked)
{
    Q_UNUSED(checked);
    if (!ui->leSearch->text().isEmpty()) //Only refresh if there is something to search
        RefreshUIDataDisplay(false, RA_SaveSelAndFocus, QList<long long>(),
                             (UIDDRefreshAction)(RA_SaveSelAndScroll | RA_NoRefreshView));
}

void MainWindow::on_action_importFirefoxBookmarks_triggered()
{
    //TODO [IMPORT]
}

void MainWindow::on_actionImportFirefoxBookmarksJSONfile_triggered()
{
    QString jsonFilePath = QFileDialog::getOpenFileName(
                this, "Import Firefox Bookmarks Backup JSON File", QString(),
                "Firefox Bookmarks Backup (bookmarks*.json)");

    if (jsonFilePath.isEmpty())
        return;

    ImportFirefoxJSONFile(jsonFilePath);
}

void MainWindow::on_actionImportUrlsAsBookmarks_triggered()
{
    long long importFOID = ui->tf->GetCurrentFOID();
    if (importFOID < 0) //Don't allow importing into '-1, All Bookmarks' and other special folders.
        importFOID = 0;
    QString importFolderName = dbm.bfs.GetPathOrName(importFOID).toHtmlEscaped();

    bool okay;
    QString message = "Enter the URLs to process, each one in a single line.<br/>"
            "Bookmarks will be imported into the folder <span style=\"color:blue;\">" + importFolderName + "</span>.";
    QString urls = QInputDialog::getMultiLineText(this, "Add URL(s) as Bookmark", message, QString(), &okay);
    if (!okay)
        return;

    QStringList urlsList = urls.split('\n', QString::KeepEmptyParts);
    if (urlsList.isEmpty())
    {
        QMessageBox::warning(this, "No URLs given", "Please enter at least one URL to import.");
        return;
    }

    ImportURLs(urlsList, importFOID);
}

void MainWindow::on_actionImportMHTFiles_triggered()
{
    long long importFOID = ui->tf->GetCurrentFOID();
    if (importFOID < 0) //Don't allow importing into '-1, All Bookmarks' and other special folders.
        importFOID = 0;
    QString importFolderName = dbm.bfs.GetPathOrName(importFOID);
    QString dlgTitle = "Import MHTML Files into Folder \"" + importFolderName + "\"";

    const QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString lastMHTBrowseDir = dbm.sets.GetSetting("LastMHTBrowseDir", documentsDir);

    QStringList mhtFilePaths = QFileDialog::getOpenFileNames(
                this, dlgTitle, lastMHTBrowseDir, "MHTML File (*.mht *.mhtml)");

    if (mhtFilePaths.isEmpty())
        return;

    QFileInfo mhtFileNameInfo(mhtFilePaths[0]);
    dbm.sets.SetSetting("LastMHTBrowseDir", mhtFileNameInfo.absolutePath());

    ImportMHTFiles(mhtFilePaths, importFOID);
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog setsDlg(&dbm, this);
    setsDlg.exec();
}

void MainWindow::InitializeUIControlsAndPositions()
{
    // Set size and position
    WindowSizeMemory::SetWindowSizeMemory(this, this, &dbm, "MainWindow", true, true, true, 0.75f);

    // Set additional UI sizes
    QList<int> hsizes, vsizes;
    hsizes << this->width() / 3 << this->width() * 2 / 3;
    ui->splitter->setSizes(hsizes);
    vsizes << this->height() * 2 / 3 << this->height() / 3;
    ui->splitterFT->setSizes(vsizes);

    // Add additional UI controls
    ui->action_importFirefoxBookmarks->setEnabled(false); //Not implemented yet.
    QMenu* menuFile = new QMenu("    &File    ");
    menuFile->addAction(ui->actionImportUrlsAsBookmarks);
    menuFile->addAction(ui->actionImportMHTFiles);
    menuFile->addSeparator();
    menuFile->addAction(ui->action_importFirefoxBookmarks);
    menuFile->addAction(ui->actionImportFirefoxBookmarksJSONfile);
    menuFile->addSeparator();
    menuFile->addAction(ui->actionSettings);

    QMenu* menuDebug = new QMenu("    &Debug    ");
    menuDebug->addAction(ui->actionGetMHT);

    QList<QMenu*> menus = QList<QMenu*>() << menuFile << menuDebug;
    foreach (QMenu* menu, menus)
    {
        QToolButton* btn = new QToolButton();
        btn->setText(menu->title());
        btn->setMenu(menu);
        btn->setPopupMode(QToolButton::InstantPopup);
        //btn->setArrowType(Qt::LeftArrow);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        ui->mainToolBar->addWidget(btn);
    }
}

void MainWindow::RefreshUIDataDisplay(bool rePopulateModels,
                                      UIDDRefreshAction bookmarksAction, const QList<long long>& selectBIDs,
                                      UIDDRefreshAction tagsAction, long long selectTID,
                                      const QList<long long>& newTIDsToCheck)
{
    //Calling this function after changes is needed, even for the bookmarks list.
    //  The model does NOT automatically update its view. Actually calling
    //  `PopulateModelsAndInternalTables` DOES invalidate the selection, etc, but the tableView
    //  is not updated.

    //[SavingSelectedBookmarkAndTag] used in BookmarksView and TagsView `::RefreshUIDataDisplay`
    //For saving the currently selected bookmark, we save its row in the model only. Saving bookmark
    //  selection is just used when editing the bookmark, so likely its row in the model won't change.
    //BUT for saving the currently selected tag, we save its TId, because editing a bookmark may add
    //  another tags and change the currently selected tag's row.
    //Note that if we re-add all the items, saving a QModelIndex doesn't work. QPersistentModelIndex
    //  may become invalid as well. Andre suggests manually iterating over the items and select what
    //  we want in these situations:
    //  https://forum.qt.io/topic/24652/how-to-retain-the-selection-when-altering-the-query-of-a-qsqlquerymodel/6

    //[RestoringScrollPositionProceedsCustomSelection] used in BookmarksView and TagsView `::RefreshUIDataDisplay`
    //This is useful for tags, where we want to ensure the selected tag is visible even if new tags
    //  are added, while keeping the previous original scroll position if possible.

    //IMPORTANT: First Manage tags, because `MainWindow::GetBookmarkFilter` function called next
    //  RELIES on tags' check state; we make sure whatever tags we wanted are checked first.
    ui->tv->RefreshUIDataDisplay(rePopulateModels, tagsAction, selectTID, newTIDsToCheck);

    //After managing tags, make the appropriate BookmarkFilter and use it for BookmarksView.
    BookmarkFilter bfilter;
    GetBookmarkFilter(bfilter);
    ui->bv->RefreshUIDataDisplay(rePopulateModels, bfilter, bookmarksAction, selectBIDs);

    //Refresh status labels.
    RefreshStatusLabels();
}

void MainWindow::GetBookmarkFilter(BookmarkFilter& bfilter)
{
    if (ui->tf->GetCurrentFOID() != -1) //'-1, All Bookmarks' shows we don't need filtering.
        bfilter.FilterSpecificFolderIDs(QSet<long long>() << ui->tf->GetCurrentFOID());

    //First we check if "All Items" is checked or not. If it's fully checked or fully unchecked,
    //  we don't filter by tags.
    if (ui->tv->areAllTagsChecked())
    {
        //Let bfilter stay clear (actually it's not clear; it has filtered folders already).
    }
    else
    {
        //"All Items" does not appear in `tagItems`. Even if it was, it was excluded by the Qt::Checked
        //  condition, as we have already checked `ui->tv->areAllTagsChecked()`.
        bfilter.FilterSpecificTagIDs(QSet<long long>::fromList(ui->tv->GetCheckedTIDs()));
    }

    //The search box
    if (!ui->leSearch->text().isEmpty())
    {
        QList<long long> foundBIDs;
        QString searchTerm = ui->leSearch->text();
        bool useRegExp = ui->chkSearchRegExp->isChecked();
        QRegularExpression re;
        if (useRegExp)
        {
            re = QRegularExpression(searchTerm, QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
            re.optimize();
        }

        const auto& model = dbm.bms.model;
        int bidIdx = dbm.bms.bidx.BID,
            nameIdx = dbm.bms.bidx.Name,
            urlsIdx = dbm.bms.bidx.URLs,
            descIdx = dbm.bms.bidx.Desc;

        //`model.match(...)` couldn't be used here because it can't search multiple columns,
        //although it had other options such as RegExp and string-in-the-middle matching.
        for (int i = 0; i < model.rowCount(); i++)
        {
            const QSqlRecord& record = model.record(i);
            if (useRegExp)
            {
                if (re.match(record.value(nameIdx).toString(), 0).hasMatch() ||
                    re.match(record.value(urlsIdx).toString(), 0).hasMatch() ||
                    re.match(record.value(descIdx).toString(), 0).hasMatch())
                    foundBIDs.append(record.value(bidIdx).toLongLong());
            }
            else
            {
                if (record.value(nameIdx).toString().contains(searchTerm, Qt::CaseInsensitive) ||
                    record.value(urlsIdx).toString().contains(searchTerm, Qt::CaseInsensitive) ||
                    record.value(descIdx).toString().contains(searchTerm, Qt::CaseInsensitive))
                    foundBIDs.append(record.value(bidIdx).toLongLong());
            }
        }

        bfilter.FilterSpecificBookmarkIDs(foundBIDs);
    }
}

void MainWindow::RefreshStatusLabels()
{
    //Filter criteria label
    QStringList twoFolderNameLocations;
    QString tagFilterString;
    QString searchCriteria;

    long long currentFOID = ui->tf->GetCurrentFOID();
    QString currentFolderName = dbm.bfs.GetPathOrName(currentFOID);
    //For the '0, Unsorted' and '-1, All Bookmarks' folders we use more natural sentences.
    if (currentFOID == -1) //'-1, All Bookmarks' folder
        twoFolderNameLocations << "" << " <span style=\"color:blue;\">in every folder</span>";
    else if (currentFOID == 0) //'0, Unsorted' folder
        twoFolderNameLocations << "<span style=\"color:blue;\">unsorted</span> " << "";
    else
        twoFolderNameLocations << "" << " in folder <span style=\"color:blue;\">" + currentFolderName.toHtmlEscaped() + "</span>";

    if (!ui->tv->areAllTagsChecked())
        tagFilterString = " tagged <span style=\"color:green;\">" + ui->tv->GetCheckedTagsNames().toHtmlEscaped() + "</span>";

    if (!ui->leSearch->text().isEmpty())
        searchCriteria = QString(" matching %1 <span style=\"color:fuchsia;\">%2</span>")
                .arg(ui->chkSearchRegExp->isChecked() ? "regular expression" : "text")
                .arg(ui->leSearch->text().toHtmlEscaped());

    ui->lblFilter->setText(QString("Showing %1bookmarks%2%3%4")
        .arg(twoFolderNameLocations[0], twoFolderNameLocations[1], tagFilterString, searchCriteria));

    //Bookmark count label
    int displayedBookmarksCount = ui->bv->GetDisplayedBookmarksCount();
    int bookmarksInFolderCount;
    dbm.bms.CountBookmarksInFolder(bookmarksInFolderCount, currentFOID);

    int totalBookmarksCount = ui->bv->GetTotalBookmarksCount();
    if (currentFOID == -1) //In '-1, All Bookmarks' mode, bookmarksInFolderCount will be 0.
        bookmarksInFolderCount = totalBookmarksCount;

    ui->lblBMCount->setText(QString("Showing %1%2%3 bookmarks%4")
        .arg(displayedBookmarksCount == bookmarksInFolderCount && displayedBookmarksCount > 0 ? "all " : "")
        .arg(displayedBookmarksCount)
        .arg(displayedBookmarksCount != bookmarksInFolderCount ? " of " + QString::number(bookmarksInFolderCount) : 0)
        .arg(currentFOID != -1 ? " in folder" : ""));
}

void MainWindow::NewBookmark()
{
    //I hesitated at allocating the modal dialog on stack, but look here for a nice discussion
    //  about message loop of modal dialogs:
    //  http://qt-project.org/forums/viewthread/15361

    BookmarkEditDialog::OutParams outParams;
    BookmarkEditDialog bmEditDialog(&dbm, -1, ui->tf->GetCurrentFOID(), &outParams, this);

    if (!bmEditDialog.canShow())
        return; //In case of errors a message is already shown.

    int result = bmEditDialog.exec();
    if (result != QDialog::Accepted)
        return;

    RefreshUIDataDisplay(true, RA_CustomSelectAndFocus, QList<long long>() << outParams.addedBId,
                         RA_SaveSelAndScrollAndCheck, -1, outParams.associatedTIDs);
}

void MainWindow::MergeSelectedBookmarks()
{
    QList<long long> mergeBIDs = ui->bv->GetSelectedBookmarkIDs();

    MergeConfirmationDialog::OutParams outParams;
    MergeConfirmationDialog mergeConfirmDialog(&dbm, mergeBIDs, &outParams, this);


    if (!mergeConfirmDialog.canShow())
        return; //In case of errors a message is already shown.

    int result = mergeConfirmDialog.exec();
    if (result != QDialog::Accepted)
        return;

    //Move the main BID to the first index.
    if (mergeBIDs[0] != outParams.mainBId)
    {
        mergeBIDs.removeAll(outParams.mainBId);
        mergeBIDs.insert(0, outParams.mainBId);
    }

    QList<long long> associatedTIDs;
    BookmarksBusinessLogic bbLogic(&dbm, this);
    bool success = bbLogic.MergeBookmarksTrans(mergeBIDs, associatedTIDs);
    if (!success)
        return;

    RefreshUIDataDisplay(true, RA_CustomSelectAndFocus, QList<long long>() << outParams.mainBId,
                         RA_SaveSelAndScrollAndCheck, -1, associatedTIDs);
}

void MainWindow::ViewSelectedBookmark()
{
    const long long BID = ui->bv->GetSelectedBookmarkIDs()[0];
    BookmarkViewDialog bmViewDialog(&dbm, BID, this);

    if (!bmViewDialog.canShow())
        return; //In case of errors a message is already shown.

    bmViewDialog.exec();

    //No need to refresh UI display.
}

void MainWindow::EditSelectedBookmark()
{
    BookmarkEditDialog::OutParams outParams;
    const long long BID = ui->bv->GetSelectedBookmarkIDs()[0];
    BookmarkEditDialog bmEditDialog(&dbm, BID, -1, &outParams, this);

    if (!bmEditDialog.canShow())
        return; //In case of errors a message is already shown.

    int result = bmEditDialog.exec();
    if (result != QDialog::Accepted)
        return;

    //20141009: The model is reset so we should do the selection manually ourselves.
    RefreshUIDataDisplay(true, RA_CustomSelAndSaveScrollAndFocus, QList<long long>() << BID,
                         RA_SaveSelAndScrollAndCheck, -1, outParams.associatedTIDs);
}

void MainWindow::DeleteSelectedBookmarks()
{
    QString selectedBookmarkNames = ui->bv->GetSelectedBookmarkNames().join("\n");

    if (QMessageBox::Yes !=
        QMessageBox::question(this, "Delete Bookmark",
                              "Are you sure you want to send the following selected bookmark(s) to the trash?\n\n"
                              + selectedBookmarkNames,
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
        return;

    BookmarksBusinessLogic bbLogic(&dbm, this);
    bool success = bbLogic.DeleteBookmarksTrans(ui->bv->GetSelectedBookmarkIDs());
    if (!success)
        return;

    //After delete, no item remains selected.
    RefreshUIDataDisplay(true, RA_SaveScrollPosAndFocus, QList<long long>(), RA_SaveSelAndScrollAndCheck);
}

void MainWindow::ImportURLs(const QStringList& urls, long long importFOID)
{
    ImportedEntityList elist;
    elist.importFOID = importFOID;
    elist.importSource = ImportedEntityList::Source_Urls;

    ImportedBookmarkFolder ibf;
    ibf.title = "";
    ibf.intId = 0;
    ibf.parentId = -1;
    elist.ibflist.append(ibf);

    int intId = 0;
    foreach (const QString& surl, urls)
    {
        QString url = surl.trimmed();
        if (url.isEmpty())
            continue;

        ImportedBookmark ib;
        ib.title = QString();
        ib.intId = intId++;
        ib.parentId = 0;
        ib.uri = url;
        elist.iblist.append(ib);
    }

    ImportBookmarks(elist);
}

void MainWindow::ImportMHTFiles(const QStringList& filePaths, long long importFOID)
{
    ImportedEntityList elist;
    elist.importFOID = importFOID;
    elist.importSource = ImportedEntityList::Source_Files;

    ImportedBookmarkFolder ibf;
    ibf.title = "";
    ibf.intId = 0;
    ibf.parentId = -1;
    elist.ibflist.append(ibf);

    int intId = 0;
    foreach (const QString& mhtFilePath, filePaths)
    {
        QFile mhtfile(mhtFilePath);
        if (!mhtfile.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(this, "Error", "Could not open the file \"" + mhtFilePath + "\".\nImporting stopped.");
            return;
        }
        QByteArray mhtData = mhtfile.readAll();
        mhtfile.close();

        QString title, url;
        MHTSaver::ExtractInfoDumb(mhtData, title, url);

        //`.simplified` is needed since e.g an i3e explore title contained newlines and tabs in it!
        title = title.simplified().trimmed();
        if (title.isEmpty())
            title = QFileInfo(mhtFilePath).completeBaseName();
        if (url.isNull())
            url = ""; //To not store null in db

        ImportedBookmark ib;
        ib.title = title;
        ib.intId = intId++;
        ib.parentId = 0;
        ib.uri = url;
        ib.ExMd_importedFilePath = mhtFilePath;
        elist.iblist.append(ib);
    }

    ImportBookmarks(elist);
}

void MainWindow::ImportFirefoxJSONFile(const QString& jsonFilePath)
{
    ImportedEntityList elist;
    elist.importFOID = 0; //Always import into the '0, Unsorted bookmarks' folder
    elist.importSource = ImportedEntityList::Source_Firefox;
    elist.importSourceFileName = QFileInfo(jsonFilePath).fileName();

    FirefoxBookmarkJSONFileParser ffParser(this, &conf);
    if (!ffParser.ParseFile(jsonFilePath, elist))
        return;

    ImportBookmarks(elist);
}

void MainWindow::ImportBookmarks(ImportedEntityList& elist)
{
    bool success;

    BookmarkImporter bmim(&dbm, this);
    success = bmim.Initialize();
    if (!success)
        return;

    success = bmim.Analyze(elist);
    if (!success)
        return;

    //Note about TRANSACTIONS:
    //Import function imports each bookmark in its own transaction. This way is both just what we
    //  did (i.e importing all of them under the same transaction was not more difficult and could
    //  be done), and also the good point about it is that if it interrupts, we'll have some of our
    //  bookmarks. So we don't need to start and wrap this in a transaction.
    success = bmim.InitializeImport();
    if (!success)
        return;

    ImportedBookmarksPreviewDialog* importPreviewDialog = new ImportedBookmarksPreviewDialog(&dbm, &bmim, &elist, this);
    success = importPreviewDialog->canShow();
    if (!success)
        return;

    int result = importPreviewDialog->exec();
    importPreviewDialog->deleteLater();

    //Don't return if rejected. Even if rejected some bookmarks may have been imported.
    //if (result != QDialog::Accepted)
    //    return;
    Q_UNUSED(result);

    QList<long long> addedBIDs;
    QSet<long long> allAssociatedTIDs;
    QList<ImportedBookmark*> failedProcessOrImports;
    bmim.FinalizeImport(addedBIDs, allAssociatedTIDs, failedProcessOrImports);

    ///Previously the preview dialog didn't import bookmarks, but now it imports them using its
    ///  bookmarks processor. So we had to import the bookmarks after it was accepted here.
    //success = bmim.Import(elist, addedBIDs, allAssociatedTIDs);
    //if (!success)
    //    return;

    //Refresh UI
    if (!addedBIDs.isEmpty())
    {
        //Show the folder into which the bookmarks are imported.
        ui->tf->SetCurrentFOIDSilently(elist.importFOID);
        RefreshUIDataDisplay(true, RA_CustomSelectAndFocus, addedBIDs,
                RA_SaveSelAndScrollAndCheck, -1, allAssociatedTIDs.toList());
    }

    //Show success message
    QString importCompleteMessage;
    if (addedBIDs.isEmpty())
        importCompleteMessage = "No bookmarks were imported.";
    else
        importCompleteMessage = QString("%1 bookmark(s) were imported.").arg(addedBIDs.size());
    if (!failedProcessOrImports.isEmpty())
        importCompleteMessage += "\n\nSome bookmarks encountered errors while importing. "
                                 "Click OK to view the error messages.";
    QMessageBox::information(this, "Import complete", importCompleteMessage);

    if (!failedProcessOrImports.isEmpty())
    {
        QString errorMessages;
        foreach (ImportedBookmark* ib, failedProcessOrImports)
            errorMessages += QString("%1 (%2):\n%3\n\n").arg(ib->title, ib->uri, ib->ExIm_finalError);
        QPlainTextEdit* ptxErrors = new QPlainTextEdit(NULL /* no parent : makes it top-level window. */);
        ptxErrors->setWindowTitle("Bookmark Processing and Import Errors");
        ptxErrors->setPlainText(errorMessages);
        ptxErrors->resize(this->size() * 0.75);
        ptxErrors->show();

        /*QRect geom = geometry();
        geom.moveCenter(QApplication::desktop()->availableGeometry().center());
        this->setGeometry(geom);*/
    }
}

class MHTDataReceiver : public QObject
{
    Q_OBJECT
public:
    MHTDataReceiver(QObject* parent = NULL) : QObject(parent) { }
    ~MHTDataReceiver() { qDebug() << "MHTDataWritten"; }
public slots:
    void MHTDataReady(const QByteArray& data, const MHTSaver::Status& status)
    {
        Q_UNUSED(status);
        qDebug() << "MHTDataReady";

        QFile mhtfile("C:\\Users\\Hossein\\Desktop\\" + status.mainResourceTitle + ".mht");
        mhtfile.open(QIODevice::WriteOnly);
        mhtfile.write(data);
        mhtfile.close();

        sender()->deleteLater();
        this->deleteLater();
    }
};
#include "MainWindow.moc" //Just for the above test mht to work (moc_MainWindow.cpp didn't work btw)

void MainWindow::on_actionGetMHT_triggered()
{
    MHTSaver* saver = new MHTSaver(this);
    saver->GetMHTData("https://docs.python.org/2/howto/unicode.html");
    MHTDataReceiver* datarecv = new MHTDataReceiver(this);
    connect(saver, SIGNAL(MHTDataReady(QByteArray,MHTSaver::Status)), datarecv, SLOT(MHTDataReady(QByteArray,MHTSaver::Status)));
}
