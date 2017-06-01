#pragma once
#include <QDialog>

#include "Database/DatabaseManager.h"

class RichRadioButtonContext;
namespace Ui { class MergeConfirmationDialog; }

class MergeConfirmationDialog : public QDialog
{
    Q_OBJECT

public:
    struct OutParams
    {
        long long mainBId;
    };

private:
    Ui::MergeConfirmationDialog *ui;
    DatabaseManager* dbm;
    bool canShowTheDialog;
    OutParams* outParams;
    RichRadioButtonContext* radioContext;

public:
    explicit MergeConfirmationDialog(DatabaseManager* dbm, const QList<long long>& BIDs,
                                     OutParams* outParams = NULL, QWidget *parent = NULL);
    ~MergeConfirmationDialog();

public:
    bool canShow();

public slots:
    void accept();
};
