#pragma once
#include <QDialog>

#include "DatabaseManager.h"

class QTableWidgetItem;
namespace Ui { class BookmarkViewDialog; }

class BookmarkViewDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::BookmarkViewDialog *ui;
    DatabaseManager* dbm;
    bool canShowTheDialog;

    BookmarkManager::BookmarkData viewBData;

public:
    explicit BookmarkViewDialog(DatabaseManager* dbm, long long viewBId = -1,
                                   QWidget *parent = 0);
    ~BookmarkViewDialog();

public:
    bool canShow();

private:
    //The following functions were copied from BookmarkEditDialog. Maybe common-ize them?

    /// Tags Section //////////////////////////////////////////////////////////////////////////////
    void PopulateUITags();

    /// File Attachments Section //////////////////////////////////////////////////////////////////
    void InitializeFilesUI();
    void PopulateUIFiles(bool saveSelection);
    void SetDefaultBFID(long long BFID);

};
