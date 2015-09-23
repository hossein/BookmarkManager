#include "OpenWithDialog.h"
#include "ui_OpenWithDialog.h"

#include "AppListItemDelegate.h"
#include "WinFunctions.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#   include <QStandardPaths>
#else
#   include <QDesktopServices>
#endif
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMenu>
#include <QToolButton>

OpenWithDialog::OpenWithDialog(DatabaseManager* dbm, const QString& fileName, bool allowNonSandbox,
                               OutParams* outParams, QWidget *parent) :
    QDialog(parent), ui(new Ui::OpenWithDialog), dbm(dbm), m_fileName(fileName),
    canShowTheDialog(false), outParams(outParams)
{
    ui->setupUi(this);
    ui->lwProgs->setItemDelegate(new AppListItemDelegate(ui->lwProgs));

    QFileInfo fileInfo(m_fileName);
    setWindowTitle(QString("Open File '%1'").arg(fileInfo.fileName()));

    ui->lwProgs->setPlaceholderText("(no program found)");

    //Don't enable until user selects something.
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    //Btw we have a 'Close' button instead of a 'Cancel' button, because we can make modifications
    //  in the OpenWithDialog to the application list and then Close it and have all the saves changed.
    //  'Cancel' is not a good name in this scenario.

    //We can't open yet un-attached files directly. We have to set both enabled AND CHECK STATUS
    //  because upon accept these values are read from the check box and in case non-sandbox is not
    //  allowed we want to make sure it's checked.
    ui->chkOpenSandboxed->setEnabled(allowNonSandbox);
    ui->chkOpenSandboxed->setChecked(true);

//TODO: These QAction shortcuts WORK NOWHERE!

    //Add a tool button with 'Rename/Delete' menus.
    typedef QKeySequence QKS;
    QMenu* optionsMenu = new QMenu("Program Menu", this);
    optionsMenu->addAction("Rena&me",           this, SLOT(pact_rename()), QKS("F2"));
    optionsMenu->addAction("Remo&ve From List", this, SLOT(pact_remove()), QKS("Del"));

    QAction* act;
    act = optionsMenu->addSeparator();
    act->setVisible(false);
    m_unassocActions.append(act);
    act = optionsMenu->addAction("Do&n't Show in Open With Menu", this, SLOT(pact_unassociate()));
    act->setVisible(false);
    m_unassocActions.append(act);

    m_optionsButton = new QToolButton(this);
    m_optionsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_optionsButton->setText("&Options");
    m_optionsButton->setPopupMode(QToolButton::InstantPopup);
    m_optionsButton->setMenu(optionsMenu);
    m_optionsButton->setEnabled(false);
    //Although this doesn't exactly set the width to that...
    m_optionsButton->setFixedWidth(ui->buttonBox->button(QDialogButtonBox::Ok)->width());

    //A "Reset" button appears at the LEFT side on all platforms on Qt4!
    ui->buttonBox->addButton(m_optionsButton, QDialogButtonBox::ResetRole);

    //Make the browse ("...") button small.
    ui->btnBrowse->setFixedWidth(20 + QFontMetrics(this->font()).width("..."));

    //Get preferred and associated apps SAIDs
    long long preferredSAID = dbm->fview.GetPreferredOpenApplication(m_fileName);
    QList<long long> associatedSAIDs = dbm->fview.GetAssociatedOpenApplications(m_fileName);

    //Add the program items
    //Default item
    m_defaultProgramItem = new QListWidgetItem();
    SetProgItemData(m_defaultProgramItem, SISAID_SystemDefaultItem, 0, true, (preferredSAID == -1),
                    QPixmap(":/res/exec32.png"), "Default System Application", QString());
    ui->lwProgs->addItem(m_defaultProgramItem);
    if (preferredSAID == -1)
        ui->lwProgs->setCurrentItem(m_defaultProgramItem);

    //Program items
    //First sort using QMap
    QMap<QString, long long> assocSortMap;
    QMap<QString, long long> unassocSortMap;
    foreach (const FileViewManager::SystemAppData& sa, dbm->fview.systemApps)
        if (associatedSAIDs.contains(sa.SAID))
            assocSortMap.insert(sa.Name.toLower(), sa.SAID);
        else
            unassocSortMap.insert(sa.Name.toLower(), sa.SAID);

    //Then display them separately
    int index = 1; //index 0 was the system default item.
    AddProgramItems(assocSortMap.values(), preferredSAID, true, index);
    AddProgramItems(unassocSortMap.values(), preferredSAID, false, index);

    //Add the browsed item.
    m_browsedProgramItem = new QListWidgetItem();
    m_browsedProgramItem->setData(AppItemRole::SAID, SISAID_BrowsedItem);
    m_browsedProgramItem->setData(AppItemRole::Index, 0);
    ui->lwProgs->addItem(m_browsedProgramItem);
    m_browsedProgramItem->setHidden(true); //Must hide AFTER adding.

    canShowTheDialog = true;
}

OpenWithDialog::~OpenWithDialog()
{
    delete ui;
}

bool OpenWithDialog::canShow()
{
    return canShowTheDialog;
}

void OpenWithDialog::accept()
{
    //If there is no selection, OK button is disabled and we don't reach here.
    //  OK button is enabled on selection or sepcial item selected.
    bool success = true;

    QListWidgetItem* selItem = ui->lwProgs->selectedItems()[0];
    long long SAID = selItem->data(AppItemRole::SAID).toLongLong();
    if (SAID == SISAID_SystemDefaultItem)
    {
        //Open with default system app?
        //Set to `NoSAID_DefaultSystemHandler` as per contract of OpenWithDialog::OutParams.
        SAID = OutParams::NoSAID_DefaultSystemHandler;
    }
    else if (SAID != SISAID_BrowsedItem)
    {
        //Program already exists. Proceed
    }
    else //if (SAID == SISAID_BrowsedItem)
    {
        //Must add the program to DB. We do it here at OpenWithDialog to show the user any
        //  errors that might happen while adding the program to database.

        //Do Not use `ui->leFilterBrowse->text()` for path because it may be relative, etc.
        //  Also we do not rely on the path being converted to native separators in other places,
        //  we convert it to native separators here anyway.
        QString browsedSystemAppPath =
                QDir::toNativeSeparators(selItem->data(AppItemRole::Path).toString());

        SAID = -1; //This is IMPORTANT for the operation of `AddOrEditSystemApp`.

        FileViewManager::SystemAppData sadata;
        sadata.SAID = SAID; //Not important.
        sadata.Name = selItem->data(AppItemRole::Name).toString();
        sadata.Path = browsedSystemAppPath;
        //Small icon: Get it fresh!
        sadata.SmallIcon = WinFunctions::GetProgramSmallIcon(browsedSystemAppPath);
        //Large icon: Already have it!
        sadata.LargeIcon = qvariant_cast<QPixmap>(selItem->data(AppItemRole::Icon));

        //This sets the SAID too.
        success = dbm->fview.AddOrEditSystemApp(SAID, sadata);
    }

    if (!success)
        return;

    if (SAID != OutParams::NoSAID_DefaultSystemHandler)
    {
        success = dbm->fview.AssociateApplicationWithExtension(m_fileName, SAID);
        if (!success)
            return;
    }

    long long oldPreferredSAID = dbm->fview.GetPreferredOpenApplication(m_fileName);
    if (ui->chkPreferProgram->isChecked() && SAID != oldPreferredSAID)
        success = dbm->fview.SetPreferredOpenApplication(m_fileName, SAID);

    if (outParams != NULL)
    {
        outParams->selectedSAID = SAID;
        outParams->openSandboxed = ui->chkOpenSandboxed->isChecked();
    }

    //Don't close the dialog if adding the systemapp encountered error.
    if (success)
        QDialog::accept();
}

void OpenWithDialog::on_leFilterBrowse_textChanged(const QString& text)
{
    Q_UNUSED(text)
    filter();
}

void OpenWithDialog::on_btnBrowse_clicked()
{
    pact_browse();
}

void OpenWithDialog::on_lwProgs_itemSelectionChanged()
{
    const bool hasSelected = !ui->lwProgs->selectedItems().isEmpty();
    const bool isSpecial = (hasSelected ? isSpecialItem(ui->lwProgs->selectedItems()[0]) : false);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasSelected);

    const bool isNormalProgramItem = hasSelected && !isSpecial;
    m_optionsButton->setEnabled(isNormalProgramItem);
    if (isNormalProgramItem)
    {
        //[Prevent Un-associating the preferred item in the UI]. This has no bad effect to the logic,
        //  just if we allow un-associating the preferred item it won't show in the Open With menu.
        //  (although we do show it bold and underlined in our item delegate).
        const bool assoc = ui->lwProgs->selectedItems()[0]->data(AppItemRole::Assoc).toBool();
        const bool pref = ui->lwProgs->selectedItems()[0]->data(AppItemRole::Pref).toBool();
        foreach (QAction* act, m_unassocActions)
            act->setVisible(assoc && !pref);
    }
}

void OpenWithDialog::on_lwProgs_itemActivated(QListWidgetItem* item)
{
    Q_UNUSED(item)
    accept();
}

void OpenWithDialog::on_lwProgs_customContextMenuRequested(const QPoint& pos)
{
    //[Clear selection on useless right-click]
    if (ui->lwProgs->itemAt(pos) == NULL)
        ui->lwProgs->clearSelection();

    //Now  check for selection.
    bool programSelected = (!ui->lwProgs->selectedItems().empty());
    if (programSelected && isSpecialItem(ui->lwProgs->selectedItems()[0]))
        return; //Do not show menu for special items.

    typedef QKeySequence QKS;
    QMenu optionsMenu("Program Menu");

    if (!programSelected)
    {
        optionsMenu.addAction("&Browse...",        this, SLOT(pact_browse()));
    }
    else
    {
        optionsMenu.addAction("Rena&me",           this, SLOT(pact_rename()), QKS("F2"));
        optionsMenu.addAction("Remo&ve From List", this, SLOT(pact_remove()), QKS("Del"));

        //[Prevent Un-associating the preferred item in the UI]
        const bool isAssociated = ui->lwProgs->selectedItems()[0]->data(AppItemRole::Assoc).toBool();
        const bool isPreferred = ui->lwProgs->selectedItems()[0]->data(AppItemRole::Pref).toBool();
        if (isAssociated && !isPreferred)
        {
            optionsMenu.addSeparator();
            optionsMenu.addAction("Do&n't Show in Open With Menu", this, SLOT(pact_unassociate()));
        }
    }

    QPoint menuPos = ui->lwProgs->viewport()->mapToGlobal(pos);
    optionsMenu.exec(menuPos);
}

bool OpenWithDialog::isSpecialItem(QListWidgetItem* item)
{
    return (item->data(AppItemRole::SAID).toLongLong() < 0);
}

void OpenWithDialog::AddProgramItems(const QList<long long>& SAIDs, long long preferredSAID,
                                     bool isAssociated, int& index)
{
    foreach (long long appSAID, SAIDs)
    {
        const FileViewManager::SystemAppData& sa = dbm->fview.systemApps[appSAID];

        QListWidgetItem* item = new QListWidgetItem();
        SetProgItemData(item, sa.SAID, index++, isAssociated, (preferredSAID == sa.SAID),
                        sa.LargeIcon, sa.Name, sa.Path);
        ui->lwProgs->addItem(item);

        if (sa.SAID == preferredSAID)
            ui->lwProgs->setCurrentItem(item);
    }
}

void OpenWithDialog::SetProgItemData(QListWidgetItem* item, long long SAID,
                                     int index, bool associated, bool preferred,
                                     const QPixmap& pixmap, const QString& text, const QString& path)
{
    item->setIcon(QIcon(pixmap));
    item->setText(text);

    item->setData(AppItemRole::Icon, pixmap); //`item->setIcon` is NOT enough.
    item->setData(AppItemRole::Name, text); //same as `item->setText(text);`
    item->setData(AppItemRole::SAID, SAID);
    item->setData(AppItemRole::Path, path);
    item->setData(AppItemRole::Index, index); //Used for alternating colorizing by AppListItemDelegate.
    item->setData(AppItemRole::Assoc, associated); //We show associated (and special) programs bold.
    item->setData(AppItemRole::Pref, preferred); //We underline preferred program.
}

int OpenWithDialog::filterItemsRoleAndSelectFirst(int role, const QString& str)
{
    int numFound = 0;
    for (int row = 0; row < ui->lwProgs->count(); row++)
    {
        QListWidgetItem* item = ui->lwProgs->item(row);
        if (isSpecialItem(item))
        {
            item->setHidden(true);
            continue;
        }

        bool containsStr = item->data(role).toString().contains(str, Qt::CaseInsensitive);
        item->setHidden(!containsStr);
        if (containsStr)
        {
            item->setData(AppItemRole::Index, numFound);
            numFound++;
            if (numFound == 1) //First found item
                ui->lwProgs->setCurrentItem(item);
        }
    }
    return numFound;
}

void OpenWithDialog::filter()
{
    //We need to control unselecting/selecting the only (maybe browsed) item ourselves to prevent
    //  buttons remaining enabled for hidden items, so we deselect everything, AND:
    //BEFORE returnING from this function always choose the first filtered item.
    ui->lwProgs->clearSelection(); //is NOT enoguh to clear the `current` index; We aren't using it
                                   //  here, but anyway we set it to an invalid index to prevent bugs.
    ui->lwProgs->setCurrentIndex(QModelIndex());

    QString text = ui->leFilterBrowse->text();

    if (text == "")
    {
        //On empty filter, show SystemDefault and other items but not the browsed item.
        for (int row = 0; row < ui->lwProgs->count(); row++)
        {
            ui->lwProgs->item(row)->setData(AppItemRole::Index, row);
            ui->lwProgs->item(row)->setHidden(false);
        }
        m_browsedProgramItem->setHidden(true);
        ui->lwProgs->setCurrentItem(ui->lwProgs->item(0)); //i.e the m_defaultProgramItem
        return;
    }

    //Process all items' texts first. Function will hide special items.
    //  If found, first item is automatically selected.
    int numFound = filterItemsRoleAndSelectFirst(AppItemRole::Name, text);
    if (numFound > 0)
        return;

    //If no results were found, all other items are hiddren, we try to see if the entered path
    //  is a path, and if it's an EXE file.
    bool systemAppFound = false;
    bool newAppFound = false;
    QFileInfo fi(text);
    if (fi.isFile())
    {
#if defined(Q_OS_WIN32)
        QString lSuffix = fi.suffix().toLower();
        if (lSuffix == "exe" || lSuffix == "com")
        {
            //We can't rely on displayName or largeIcon being empty for error handling.
            //  If the program hasn't set a name in its resources, GetProgramDisplayName returns
            //  the program name. Similarly the program simply may not have a large icon in it.
            //  The above two are valid mostly for command-line programs.
            //Note that we don't reach here if the program path is already in DB; in that case the
            //  existing record is already found and user's custom display name is already used.
            QString absoluteFilePath = QDir::toNativeSeparators(fi.absoluteFilePath());

            //Search to make sure we don't have that program PATH already.
            int progPathFound = filterItemsRoleAndSelectFirst(AppItemRole::Path, absoluteFilePath);
            if (progPathFound == 0)
            {
                newAppFound = true;

                //New program, get its display name and icon and display it.
                QString displayName = WinFunctions::GetProgramDisplayName(absoluteFilePath);
                QPixmap largeIcon = WinFunctions::GetProgramLargeIcon(absoluteFilePath);
                SetProgItemData(m_browsedProgramItem, SISAID_BrowsedItem,
                                0, true /*Show it BOLD*/, false /*but not underlined*/,
                                largeIcon, displayName, absoluteFilePath);
            }
            else
            {
                systemAppFound = true;
                //The above filter function already displays and selects the found app.
            }

        }
#else
#   error On unix etc must check if the file is executable by other means
#endif
    }

    m_browsedProgramItem->setHidden(!newAppFound);

    if (newAppFound)
    {
        ui->lwProgs->setCurrentItem(m_browsedProgramItem);
    }
    else if (!systemAppFound)
    {
        //We have a ListWidgetWithEmptyPlaceholder, not a simple QListWidget so it will show
        //  'no program found' here!
        //We couldn't use a disabled, unselectable item for this purpose. It didn't show up e.g
        //  when deleting the only filtered program and maybe when an invalid file name was entered
        //  into the filter box.
    }
}

void OpenWithDialog::pact_browse()
{
#if defined(Q_OS_WIN32)
    //Note: BAT files don't have icon, etc. They are a more general idea than supporting arbitrary
    //      parameters for executables. However we need extra code to support them, so for now we
    //      forget them. Maybe in the future we do a full custom command-line Open With.
    //Note: We don't need to set the *.lnk extension as the filter for the open dialog below, Qt
    //      handles them. (Start Menu programs (the default directory) are all shortcuts.)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString programsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
#else
    QString programsDir = QDesktopServices::storageLocation(QDesktopServices::ApplicationsLocation);
#endif
    QString exeFileName = QFileDialog::getOpenFileName(this, "Select Program", programsDir,
                                                       "Executables (*.exe *.com)");
    if (exeFileName.length() > 0)
        ui->leFilterBrowse->setText(exeFileName);
#else
#   error On unix etc how should we browse for executables? i mean, most use Desktop files!
#endif
}

void OpenWithDialog::pact_rename()
{
    //TODO: Seems there is a NULL byte at the end of the prog names. Try putting sth after them.

    //We can't reach here if there is no item selected, as the 'Rename' menu would be disabled.
    QListWidgetItem* selItem = ui->lwProgs->selectedItems()[0];
    long long SAID = selItem->data(AppItemRole::SAID).toLongLong();

    FileViewManager::SystemAppData& sadata = dbm->fview.systemApps[SAID];
    bool ok;
    QString newName = QInputDialog::getText(
                        this, "Rename Application", "Enter the new name for the application:",
                        QLineEdit::Normal, sadata.Name, &ok);

    if (!ok)
        return;

    sadata.Name = newName;
    bool success = dbm->fview.AddOrEditSystemApp(SAID, sadata);

    if (success)
        selItem->setData(AppItemRole::Name, newName);
}

void OpenWithDialog::pact_remove()
{
    QListWidgetItem* selItem = ui->lwProgs->selectedItems()[0];
    long long SAID = selItem->data(AppItemRole::SAID).toLongLong();
    QString appName = selItem->data(AppItemRole::Name).toString();

    int result = QMessageBox::question(
                this, "Remove Application From List",
                QString("Remove '%1' from list of commonly used applications?").arg(appName),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result != QMessageBox::Yes)
        return;

    QList<long long> myAssocSAIDs = dbm->fview.GetAssociatedOpenApplications(m_fileName);
    long long myPrefSAID = dbm->fview.GetPreferredOpenApplication(m_fileName);

    QStringList assocExts = dbm->fview.GetExtensionsWithWhichApplicationIsAssociated(SAID);
    QStringList prefExts = dbm->fview.GetExtensionsForWhichApplicationIsPreffered(SAID);

    QString alreadyAssocPrefError;

    const bool associatedWithMyFileType = myAssocSAIDs.contains(SAID);
    if (associatedWithMyFileType && assocExts.size() == 1)
    {
        //Only associated with this app. Don't warn the user.
    }
    else if (assocExts.size() > 0)
    {
        QString includingThisFileType = associatedWithMyFileType ? " (including this file type)" : "";
        alreadyAssocPrefError +=
            QString("The program '%1' is commonly used to open the following file types%2:\n%3\n\n")
            .arg(appName, includingThisFileType, assocExts.join(" "));
    }

    if (prefExts.size() > 0)
    {
        //Only preferred to open the current file type.
        if (alreadyAssocPrefError.length() == 0)
            alreadyAssocPrefError += QString("The program '%1' ").arg(appName);
        else
            alreadyAssocPrefError += "In addition, it ";

        alreadyAssocPrefError += "is used as the preferred application to open ";

        if (myPrefSAID == SAID && prefExts.size() == 1)
        {
            //Only preferred for current file type.
            alreadyAssocPrefError += "the current file type.\n";
        }
        else
        {
            QString includingThisFileType = (myPrefSAID == SAID) ? " (including this file type)" : "";
            alreadyAssocPrefError +=
                QString("the following file types%1:\n%2\n")
                .arg(includingThisFileType, prefExts.join(" "));
        }

        alreadyAssocPrefError += "If you remove it from this list, the file types that open with it "
                            "will default to opening with the default system application.\n\n";
    }

    if (alreadyAssocPrefError.length() > 0)
    {
        alreadyAssocPrefError += "Are you sure you want to delete it from the list?";

        int result = QMessageBox::question(
                    this, "Remove Application From List", alreadyAssocPrefError,
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (result != QMessageBox::Yes)
            return;
    }

    //Finally user chose to delete the program.
    dbm->fview.DeleteSystemAppAndAssociationsAndPreference(SAID);

    //Now remove the selItem too, AND: if it was the preferred application set the System Default
    //  item as the preferred thing (this is what the DeleteSystemApp... function does), and select
    //  it for more emphasis too. Otherwise the selection is just cleared.
    delete selItem;

    if (myPrefSAID == SAID)
    {
        m_defaultProgramItem->setData(AppItemRole::Pref, true);

        //IMPORTANT: Only select it if visible, i.e not filtered; otherwise I expected that we end up
        //  with no selection visible and the 'Options' and 'OK' buttons enabled, but we really end up
        //  with the first item in the filtered list selected, so we deselect everything manually.
        if (!m_defaultProgramItem->isHidden())
            ui->lwProgs->setCurrentItem(m_defaultProgramItem);
        else
            ui->lwProgs->setCurrentIndex(QModelIndex());
    }
    else
    {
        //Clear selection on removing the preferred app, generally.
        ui->lwProgs->setCurrentIndex(QModelIndex());
    }
}

void OpenWithDialog::pact_unassociate()
{
    //We can't reach here if there is no item selected, as the 'Don't Show...' menu would be disabled.
    QListWidgetItem* selItem = ui->lwProgs->selectedItems()[0];
    long long SAID = selItem->data(AppItemRole::SAID).toLongLong();

    bool success = dbm->fview.UnAssociateApplicationWithExtension(m_fileName, SAID);

    //Note that we do NOT sort the applications again afterwards; the newly un-associated item
    //  remains in its place, but with a non-bold font.
    if (success)
        selItem->setData(AppItemRole::Assoc, false);
}
