#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "Config.h"
#include "DatabaseManager.h"

SettingsDialog::SettingsDialog(DatabaseManager* dbm, QWidget *parent) :
    QDialog(parent), ui(new Ui::SettingsDialog), dbm(dbm)
{
    ui->setupUi(this);

    bool FsTransformUnicode = dbm->sets.GetSettingBool("FsTransformUnicode",
                                                       dbm->conf->defaultFsTransformUnicode);
    ui->chkFsTransformUnicode->setChecked(FsTransformUnicode);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accept()
{
    //Error messages will be shown by SettingsManager in case of errors.

    if (!dbm->sets.SetSetting("FsTransformUnicode", ui->chkFsTransformUnicode->isChecked()))
        return;

    QDialog::accept();
}
