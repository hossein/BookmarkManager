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

private slots:
    void on_twAttachedFiles_customContextMenuRequested(const QPoint& pos);

    //The following functions were copied from BookmarkEditDialog. Maybe common-ize them?
    /// Tags Section //////////////////////////////////////////////////////////////////////////////
    void PopulateUITags();

    /// File Attachments Section //////////////////////////////////////////////////////////////////
    void InitializeFilesUI();
    void PopulateUIFiles(bool saveSelection);
    void SetDefaultBFID(long long BFID);
    /// Returns -1 if no default file.
    int DefaultFileIndex();

    //Attached files actions.
    void af_preview();
    void af_open();
    void af_edit();
    void af_openWith();
    void af_properties();
    QString GetAttachedFileFullPathName(int filesListIdx);

    /// File Preview Section //////////////////////////////////////////////////////////////////////
    void PreviewFile(int index);
};
