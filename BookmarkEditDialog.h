#pragma once
#include <QDialog>

#include "DatabaseManager.h"

class FileManager;

namespace Ui { class BookmarkEditDialog; }

class BookmarkEditDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::BookmarkEditDialog *ui;
    DatabaseManager* dbm;
    bool canShowTheDialog;
    long long editBId;
    BookmarkManager::BookmarkData editOriginalBData;

public:
    explicit BookmarkEditDialog(DatabaseManager* dbm, long long editBId = -1, QWidget *parent = 0);
    ~BookmarkEditDialog();

public:
    bool canShow();

public slots:
    void accept();

private slots:
    /// UI Things /////////////////////////////////////////////////////////////////////////////////
    void on_dialRating_valueChanged(int value);
    void on_dialRating_sliderMoved(int position);
    void on_dialRating_dialReleased();

    /// Tags Section //////////////////////////////////////////////////////////////////////////////
    void PopulateUITags();

    /// File Attachments Section //////////////////////////////////////////////////////////////////
    void PopulateUIFiles();

    void on_btnShowAttachUI_clicked();
    void on_tvAttachedFiles_activated(const QModelIndex &index);
    void tvAttachedFilesCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous);
    void on_tvAttachedFiles_customContextMenuRequested(const QPoint &pos);

    void on_btnBrowse_clicked();
    void on_btnAttach_clicked();
    void on_btnCancelAttach_clicked();
};
