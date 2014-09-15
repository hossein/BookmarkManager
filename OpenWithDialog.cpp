#include "OpenWithDialog.h"
#include "ui_OpenWithDialog.h"

#include <QFontMetrics>
#include <QMenu>
#include <QToolButton>

OpenWithDialog::OpenWithDialog(DatabaseManager* dbm, QWidget *parent) :
    QDialog(parent), ui(new Ui::OpenWithDialog), dbm(dbm), canShowTheDialog(false)
{
    ui->setupUi(this);

    //Add a tool button with 'Rename/Delete' menus.
    typedef QKeySequence QKS;
    QMenu* optionsMenu = new QMenu("Program Menu", this);
    optionsMenu->addAction("&Rename", this, SLOT(pact_rename()), QKeySequence("F2"));
    optionsMenu->addAction("&Remove From List", this, SLOT(pact_remove()), QKeySequence("Del"));

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

    //TODO
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

}

void OpenWithDialog::on_lwProgs_customContextMenuRequested(const QPoint& pos)
{

}

void OpenWithDialog::browse()
{

}

void OpenWithDialog::filter()
{

}

void OpenWithDialog::pact_rename()
{

}

void OpenWithDialog::pact_remove()
{

}
