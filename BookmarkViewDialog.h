#pragma once
#include <QDialog>

#include "DatabaseManager.h"

class Config;
class QTableWidgetItem;
namespace Ui { class BookmarkViewDialog; }

class BookmarkViewDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::BookmarkViewDialog *ui;
    DatabaseManager* dbm;
    Config* conf;
    bool canShowTheDialog;
    int twAttachedFilesRequiredHeight;

    BookmarkManager::BookmarkData viewBData;

public:
    explicit BookmarkViewDialog(DatabaseManager* dbm, Config* conf, long long viewBId = -1,
                                QWidget *parent = 0);
    ~BookmarkViewDialog();

protected:
    void resizeEvent(QResizeEvent* event);

public:
    bool canShow();

private slots:
    void on_twAttachedFiles_itemActivated(QTableWidgetItem *item);
    void on_twAttachedFiles_itemSelectionChanged();
    void on_twAttachedFiles_customContextMenuRequested(const QPoint& pos);

    /// File Properties section
    void on_btnOpenUrl_clicked();

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
    void af_open();
    void af_edit();
    void af_openWith();
    void af_properties();
    QString GetAttachedFileFullPathName(int filesListIdx);

    /// Linked Bookmarks Section //////////////////////////////////////////////////////////////////
    void InitializeLinkedBookmarksUI();
    void PopulateLinkedBookmarks();

    void bvLinkedBookmarksActivated(long long BID);

    /// Extra Info Section ////////////////////////////////////////////////////////////////////////
    void PopulateExtraInfos();

    /// File Preview Section //////////////////////////////////////////////////////////////////////
    void PreviewFile(int index);
};
