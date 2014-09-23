#include "OpenWithDialog.h"
#include "ui_OpenWithDialog.h"

#include "AppListItemDelegate.h"
#include "WinFunctions.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMenu>
#include <QToolButton>

OpenWithDialog::OpenWithDialog(DatabaseManager* dbm, const QString& fileName,
                               OutParams* outParams, QWidget *parent) :
    QDialog(parent), ui(new Ui::OpenWithDialog), dbm(dbm), m_fileName(fileName),
    canShowTheDialog(false), outParams(outParams)
{
    ui->setupUi(this);
    ui->lwProgs->setItemDelegate(new AppListItemDelegate(ui->lwProgs));

    QFileInfo fileInfo(m_fileName);
    setWindowTitle(QString("Open File '%1'").arg(fileInfo.fileName()));

    //Don't enable until user selects something.
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

//TODO: These QAction shortcuts WORK NOWHERE!

    //Add a tool button with 'Rename/Delete' menus.
    typedef QKeySequence QKS;
    QMenu* optionsMenu = new QMenu("Program Menu", this);
    optionsMenu->addAction("Rena&me",           this, SLOT(pact_rename()), QKS("F2"));
    optionsMenu->addAction("Remo&ve From List", this, SLOT(pact_remove()), QKS("Del"));

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

    //Get default app SAID
    long long preferredSAID = dbm->fview.GetPreferredOpenApplication(m_fileName);

    //Add the program items
    //Default item
    m_defaultProgramItem = new QListWidgetItem();
    setProgItemData(m_defaultProgramItem, SISAID_SystemDefaultItem, 0,
                    QPixmap(":/res/exec32.png"), "Default System Application", QString());
    ui->lwProgs->addItem(m_defaultProgramItem);
    if (preferredSAID == -1)
        ui->lwProgs->setCurrentItem(m_defaultProgramItem);

    //Program items
    //First sort using QMap
    QMap<QString, long long> sortMap;
    foreach (const FileViewManager::SystemAppData& sa, dbm->fview.systemApps)
        sortMap.insert(sa.Name.toLower(), sa.SAID);

    //Then display them
    int index = 1; //index 0 was the system default item.
    foreach (long long appSAID, sortMap.values())
    {
        const FileViewManager::SystemAppData& sa = dbm->fview.systemApps[appSAID];

        QListWidgetItem* item = new QListWidgetItem();
        setProgItemData(item, sa.SAID, index++, sa.LargeIcon, sa.Name, sa.Path);
        ui->lwProgs->addItem(item);

        if (sa.SAID == preferredSAID)
            ui->lwProgs->setCurrentItem(item);
    }

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
    bool hasSelected = !ui->lwProgs->selectedItems().isEmpty();
    bool isSpecial = (hasSelected ? isSpecialItem(ui->lwProgs->selectedItems()[0]) : false);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasSelected);
    m_optionsButton->setEnabled(hasSelected && !isSpecial);
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
    }

    QPoint menuPos = ui->lwProgs->viewport()->mapToGlobal(pos);
    optionsMenu.exec(menuPos);
}

bool OpenWithDialog::isSpecialItem(QListWidgetItem* item)
{
    return (item->data(AppItemRole::SAID).toLongLong() < 0);
}

void OpenWithDialog::setProgItemData(QListWidgetItem* item, long long SAID, int index,
                                     const QPixmap& pixmap, const QString& text, const QString& path)
{
    item->setIcon(QIcon(pixmap));
    item->setText(text);

    item->setData(AppItemRole::Icon, pixmap); //`item->setIcon` is NOT enough.
    item->setData(AppItemRole::Name, text); //same as `item->setText(text);`
    item->setData(AppItemRole::SAID, SAID);
    item->setData(AppItemRole::Path, path);
    item->setData(AppItemRole::Index, index); //Used for alternating colorizing by AppListItemDelegate.
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
        if (fi.suffix().toLower() == "exe")
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
                setProgItemData(m_browsedProgramItem, SISAID_BrowsedItem, 0,
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
        //NOTE: Showing a 'no program found' is more beautiful!
    }
}

void OpenWithDialog::pact_browse()
{
#if defined(Q_OS_WIN32)
    //NOTE: BAT files don't have icon, etc. They are a more general idea than supporting arbitrary
    //      parameters for executables. However we should also set the *.lnk extension as Start Menu
    //      programs (the default directory) are all shortcuts.
    QString programsDir = QDesktopServices::storageLocation(QDesktopServices::ApplicationsLocation);
    QString exeFileName = QFileDialog::getOpenFileName(this, "Select Program", programsDir,
                                                       "Executables (*.exe *.com *.bat)");
    if (exeFileName.length() > 0)
        ui->leFilterBrowse->setText(exeFileName);
#else
#   error On unix etc how should we browse for executables? i mean, most use Desktop files!
#endif
}

void OpenWithDialog::pact_rename()
{
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
    //TODO
    //TODO: Now do we want to show associated apps on top? And what does this 'Remove' is supposed to do?
}
