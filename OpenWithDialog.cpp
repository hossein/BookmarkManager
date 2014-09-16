#include "OpenWithDialog.h"
#include "ui_OpenWithDialog.h"

#include <QFileDialog>
#include <QFontMetrics>
#include <QMenu>
#include <QToolButton>

OpenWithDialog::OpenWithDialog(DatabaseManager* dbm, QWidget *parent) :
    QDialog(parent), ui(new Ui::OpenWithDialog), dbm(dbm), canShowTheDialog(false)
{
    ui->setupUi(this);

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
        QString saText = QString("<b>%1</b><br/>%2").arg(sa.Name, sa.Path);
        QListWidgetItem* item = new QListWidgetItem(QIcon(sa.LargeIcon), saText, ui->lwProgs);
    }

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
    QDialog::accept();
}

void OpenWithDialog::on_leFilterBrowse_textEdited(const QString &text)
{
    Q_UNUSED(text)
    filter();
}

void OpenWithDialog::on_btnBrowse_clicked()
{
    browse();
}

void OpenWithDialog::on_lwProgs_itemSelectionChanged()
{
    m_optionsButton->setEnabled(!ui->lwProgs->selectedItems().isEmpty());
}

void OpenWithDialog::on_lwProgs_itemActivated(QListWidgetItem* item)
{
    accept();
}

void OpenWithDialog::on_lwProgs_customContextMenuRequested(const QPoint& pos)
{
    //[Clear selection on useless right-click]
    if (ui->lwProgs->itemAt(pos) == NULL)
        ui->lwProgs->clearSelection();

    //Now  check for selection.
    long long SAID;
    if (ui->lwProgs->selectedItems().empty())
        SAID = -1;
    else
        SAID = ui->lwProgs->selectedItems()[0]->data(Qt::UserRole).toLongLong();
    bool programSelected = (SAID != -1);

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

void OpenWithDialog::filter()
{

}

void OpenWithDialog::pact_browse()
{

}

void OpenWithDialog::pact_rename()
{

}

void OpenWithDialog::pact_remove()
{

}
