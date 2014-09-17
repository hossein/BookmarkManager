#include "OpenWithDialog.h"
#include "ui_OpenWithDialog.h"

#include "AppListItemDelegate.h"
#include "WinFunctions.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMenu>
#include <QToolButton>

OpenWithDialog::OpenWithDialog(DatabaseManager* dbm, OutParams* outParams, QWidget *parent) :
    QDialog(parent), ui(new Ui::OpenWithDialog), dbm(dbm),
    canShowTheDialog(false), outParams(outParams)
{
    ui->setupUi(this);
    ui->lwProgs->setItemDelegate(new AppListItemDelegate(ui->lwProgs));

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

    foreach (const FileViewManager::SystemAppData& sa, dbm->fview.systemApps)
    {
        QListWidgetItem* item = new QListWidgetItem();
        setProgItemData(item, sa.SAID, sa.LargeIcon, sa.Name, sa.Path);
        ui->lwProgs->addItem(item);
    }

    m_browsedProgramItem = new QListWidgetItem();
    m_browsedProgramItem->setData(Qt::UserRole, -1);
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
    //  OK button is enabled on selection or available browsing selected.
    if (outParams != NULL)
    {
        QListWidgetItem* selItem = ui->lwProgs->selectedItems()[0];
        outParams->selectedSAID = selItem->data(Qt::UserRole+0).toLongLong();
        //Do Not use `ui->leFilterBrowse->text()` for path because it may be relative, etc.
        //  Also we do not rely on the path being converted to native separators in other places,
        //  we convert it to native separators here anyway.
        outParams->browsedSystemAppPath =
                QDir::toNativeSeparators(selItem->data(Qt::UserRole+1).toString());
    }

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
    bool isBrowsed = (hasSelected
                      ? ui->lwProgs->selectedItems()[0]->data(Qt::UserRole+0).toLongLong() == -1
                      : false);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasSelected);
    m_optionsButton->setEnabled(hasSelected && !isBrowsed);
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
    long long SAID;
    bool programSelected = (!ui->lwProgs->selectedItems().empty());
    if (!programSelected)
        SAID = -1;
    else
        SAID = ui->lwProgs->selectedItems()[0]->data(Qt::UserRole).toLongLong();

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

void OpenWithDialog::setProgItemData(QListWidgetItem* item, long long SAID,
                                     const QPixmap& pixmap, const QString& text, const QString& path)
{
    item->setIcon(QIcon(pixmap));
    item->setText(text + "\n" + path);

    item->setData(Qt::DecorationRole, pixmap);
    item->setData(Qt::DisplayRole, text);
    item->setData(Qt::UserRole+0, SAID);
    item->setData(Qt::UserRole+1, path);
}

void OpenWithDialog::lwProgsShowAllNonBrowsedItems()
{
    for (int row = 0; row < ui->lwProgs->count(); row++)
        ui->lwProgs->item(row)->setHidden(true);
    m_browsedProgramItem->setHidden(false);
}

void OpenWithDialog::lwProgsShowOnlyBrowsedItem()
{
    for (int row = 0; row < ui->lwProgs->count(); row++)
        ui->lwProgs->item(row)->setHidden(false);
    m_browsedProgramItem->setHidden(true);
}

void OpenWithDialog::filter()
{
    //TODO: Need to control unselecting/selecting the only (browsed) item ourselves?

    QString text = ui->leFilterBrowse->text();

    //Process all items first
    int numFound = 0;
    for (int row = 0; row < ui->lwProgs->count(); row++)
    {
        QListWidgetItem* item = ui->lwProgs->item(row);
        bool containsText = item->text().contains(text, Qt::CaseInsensitive);
        item->setHidden(!containsText);
        if (containsText && item != m_browsedProgramItem)
            numFound++;
    }

    //In case of found results, hide the browsed item (we must do it as it may contain text from
    //  previous browsings).
    if (numFound > 0)
    {
        m_browsedProgramItem->setHidden(true);
        return;
    }

    //If no results were found, all other items are hiddren, we try to see if the entered path
    //  is a path, and if it's an EXE file.
    bool programFound = false;
    QFileInfo fi(text);
    if (fi.isFile())
    {
#if defined(Q_OS_WIN32)
        if (fi.suffix().toLower() == "exe")
        {
            programFound = true;

            //We can't rely on displayName or largeIcon being empty for error handling.
            //  If the program hasn't set a name in its resources, GetProgramDisplayName returns
            //  the program name. Similarly the program simply may not have a large icon in it.
            //  The above two are valid mostly for command-line programs.
            //Note that we don't reach here if the program path is already in DB; in that case the
            //  existing record is already found and user's custom display name is already used.
            QString absoluteFilePath = QDir::toNativeSeparators(fi.absoluteFilePath());
            QString displayName = WinFunctions::GetProgramDisplayName(absoluteFilePath);
            QPixmap largeIcon = WinFunctions::GetProgramLargeIcon(absoluteFilePath);

            setProgItemData(m_browsedProgramItem, -1, largeIcon, displayName, absoluteFilePath);
        }
#else
#   error On unix etc must check if the file is executable by other means
#endif
    }

    m_browsedProgramItem->setHidden(!programFound);

    if (!programFound)
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

}

void OpenWithDialog::pact_remove()
{

}
