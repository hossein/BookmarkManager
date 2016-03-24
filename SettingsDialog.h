#pragma once
#include <QDialog>

class DatabaseManager;
namespace Ui { class SettingsDialog; }

class SettingsDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::SettingsDialog *ui;
    DatabaseManager* dbm;

public:
    explicit SettingsDialog(DatabaseManager* dbm, QWidget *parent = 0);
    ~SettingsDialog();

public slots:
    virtual void accept();

};
