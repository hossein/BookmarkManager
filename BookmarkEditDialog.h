#pragma once
#include <QDialog>

#include "DatabaseManager.h"

class FileManager;
class QTableWidgetItem;
namespace Ui { class BookmarkEditDialog; }

class BookmarkEditDialog : public QDialog
{
    Q_OBJECT

public:
    struct OutParams
    {
        long long addedBId;
        QList<long long> associatedTIDs;
    };

private:
    Ui::BookmarkEditDialog *ui;
    DatabaseManager* dbm;
    bool canShowTheDialog;
    OutParams* outParams;
    long long originalEditBId;
    long long editBId;
    //The contents of this MUST NOT CHANGE during data editing in the dialog.
    BookmarkManager::BookmarkData editOriginalBData;

    QList<long long> editedLinkedBookmarks;
    //QList<BookmarkManager::BookmarkExtraInfoData> editedExtraInfos; We use model instead.
    QList<FileManager::BookmarkFile> editedFilesList;
    //Note: We don't use this, since new files that will be added all have BFID=-1 so we use a
    //      field inside the `FileManager::BookmarkFile` struct instead.
    //long long editedDefBFID;

public:
    explicit BookmarkEditDialog(DatabaseManager* dbm, long long editBId = -1,
                                OutParams* outParams = NULL, QWidget *parent = 0);
    ~BookmarkEditDialog();

public:
    bool canShow();

private:
    /// Validation before acception.
    bool validate();

public slots:
    void accept();

private slots:
    /// UI Things /////////////////////////////////////////////////////////////////////////////////
    void on_dialRating_valueChanged(int value);
    void on_dialRating_sliderMoved(int position);
    void on_dialRating_sliderReleased();

    /// Tags Section //////////////////////////////////////////////////////////////////////////////
    void InitializeTagsUI();
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
    void af_showAttachUI();
    void af_previewOrOpen();
    void af_preview();
    void af_open();
    void af_edit();
    void af_openWith();
    void af_saveAs();
    void af_setAsDefault();
    void af_rename();
    void af_remove();
    void af_properties();
    /// Unlike its name, this function gets both attached and non-attached files full path.
    /// This is why it's there!
    QString GetAttachedFileFullPathName(int filesListIdx);

    /// Linked Bookmarks Section //////////////////////////////////////////////////////////////////
    void InitializeLinkedBookmarksUI();
    void PopulateLinkedBookmarks();

    void bvLinkedBookmarksCurrentRowChanged(long long currentBID, long long previousBID);
    void on_btnLinkBookmark_clicked();
    void on_btnRemoveLink_clicked();

    /// Extra Info Section ////////////////////////////////////////////////////////////////////////
    void InitializeExtraInfosUI();
    void PopulateExtraInfos();

    void tvExtraInfosCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous);
    void on_btnAddExtraInfo_clicked();
    void on_btnRemoveExtraInfo_clicked();
};
