#pragma once
#include <QDialog>
#include "DatabaseManager.h"
#include "BookmarkImporters/ImportedEntity.h"
#include <QMap>

class QTreeWidgetItem;
namespace Ui { class ImportedBookmarksPreviewDialog; }

/// This class also configures the to-be-imported bookmarks, not just preview them.
class ImportedBookmarksPreviewDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::ImportedBookmarksPreviewDialog *ui;
    DatabaseManager* dbm;
    Config* conf;
    bool canShowTheDialog;
    ImportedEntityList* elist;

    QMap<int, QTreeWidgetItem*> folderItems;
    QMap<int, QTreeWidgetItem*> bookmarkItems;
    //QMap<int, ImportedBookmarkFolder*> importedFolders;
    //QMap<int, ImportedBookmarkFolder*> importedFolders;'

    enum TreeWidgetItemData
    {
        TWID_IsFolder = Qt::UserRole,
        TWID_Index,
    };

public:
    explicit ImportedBookmarksPreviewDialog(DatabaseManager* dbm, Config* conf,
                                            ImportedEntityList* elist, QWidget *parent = 0);
    ~ImportedBookmarksPreviewDialog();

public:
    bool canShow();

public slots:
    void accept();

private slots:
    void on_twBookmarks_itemSelectionChanged();

    //For applying user selections to items, we use slots that do NOT fire programmatically.
    void on_leTagsForAll_editingFinished();
    void on_chkImportBookmark_clicked();
    void on_leTagsForBookmark_editingFinished();
    void on_optKeep_clicked();
    void on_optOverwrite_clicked();
    void on_optAppend_clicked();
    void on_chkImportFolder_clicked();
    void on_leTagsForFolder_editingFinished();

private:
    void AddItems();
    void RecursiveSetFolderImport(QTreeWidgetItem* twi, bool import);
};
