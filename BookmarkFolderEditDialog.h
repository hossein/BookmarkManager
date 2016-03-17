#pragma once
#include <QDialog>

#include "DatabaseManager.h"

namespace Ui { class BookmarkFolderEditDialog; }

class BookmarkFolderEditDialog : public QDialog
{
    Q_OBJECT

public:
    struct OutParams
    {
        long long addedFOID;
    };

private:
    Ui::BookmarkFolderEditDialog *ui;
    DatabaseManager* dbm;
    bool canShowTheDialog;
    OutParams* outParams;
    long long editFOID;
    long long addParentFOID;
    QStringList siblingNames;
    BookmarkFolderManager::BookmarkFolderData editOriginalFoData;

public:
    explicit BookmarkFolderEditDialog(DatabaseManager* dbm,
                                      long long editFOID = -1, long long addParentFOID = -1,
                                      OutParams* outParams = NULL, QWidget *parent = 0);
    ~BookmarkFolderEditDialog();

public:
    bool canShow();

private:
    /// Validation before acception.
    bool validate();

public slots:
    void accept();

};
