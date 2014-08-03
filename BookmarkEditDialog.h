#pragma once
#include <QDialog>

#include "DatabaseManager.h"

class FileManager;
class QTableWidgetItem;
namespace Ui { class BookmarkEditDialog; }

class BookmarkEditDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::BookmarkEditDialog *ui;
    DatabaseManager* dbm;
    bool canShowTheDialog;
    long long* reportAddedBId;
    long long originalEditBId;
    long long editBId;
    //The contents of this MUST NOT CHANGE during data editing in the dialog.
    BookmarkManager::BookmarkData editOriginalBData;

    QList<FileManager::BookmarkFile> editedFilesList;
    //Note: We don't use this, since new files that will be added all have BFID=-1 so we use a
    //      field inside the `FileManager::BookmarkFile` struct instead.
    //long long editedDefBFID;

public:
    explicit BookmarkEditDialog(DatabaseManager* dbm, long long editBId = -1,
                                long long* resultAddedBId = NULL, QWidget *parent = 0);
    ~BookmarkEditDialog();

public:
    bool canShow();

public slots:
    void accept();
private slots:
    /// Rolls back transactions and shows error if they failed, also sets editBId = -1.
    void DoRollBackAction();

private slots:
    /// UI Things /////////////////////////////////////////////////////////////////////////////////
    void on_dialRating_valueChanged(int value);
    void on_dialRating_sliderMoved(int position);
    void on_dialRating_dialReleased();

    /// Tags Section //////////////////////////////////////////////////////////////////////////////
    void PopulateUITags();

    /// File Attachments Section //////////////////////////////////////////////////////////////////
    void InitializeFilesUI();
    void PopulateUIFiles(bool saveSelection);

    void SetDefaultBFID(long long BFID);
    void SetDefaultFileToIndex(int bfIdx);
    /// The following functions return -1 if no file is default, or there are no files.
    long long DefaultBFID();
    int DefaultFileIndex();

    void on_btnShowAttachUI_clicked();
    void on_btnSetFileAsDefault_clicked();
    void on_twAttachedFiles_itemActivated(QTableWidgetItem* item);
    void on_twAttachedFiles_itemSelectionChanged();
    void on_twAttachedFiles_customContextMenuRequested(const QPoint& pos);
    //void on_tvAttachedFiles_activated(const QModelIndex &index);
    //void tvAttachedFilesCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous);
    //void on_tvAttachedFiles_customContextMenuRequested(const QPoint& pos);

    void on_btnBrowse_clicked();
    void on_btnAttach_clicked();
    void on_btnCancelAttach_clicked();
    void ClearAndSwitchToAttachedFilesTab();

    //Attached files actions.
    void af_previewOrOpen();
    void af_preview();
    void af_open();
    void af_openWith();
    void af_setAsDefault();
    void af_rename();
    void af_remove();
    void af_properties();
};
