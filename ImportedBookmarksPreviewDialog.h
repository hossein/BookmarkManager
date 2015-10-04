#pragma once
#include <QDialog>

#include "DatabaseManager.h"
#include "BookmarkImporters/ImportedEntity.h"

#include <QIcon>
#include <QMap>

//TODOs:
//- Show errors of all those whose imports failed.
//- I removed Yevpatoria wikipedia bm but on import it nagged about similar things!
//  I removed its google search and thing got alright!

class QTreeWidgetItem;
class BookmarkImporter;
class ImportedBookmarksProcessor;
namespace Ui { class ImportedBookmarksPreviewDialog; }

/// This dialog also configures AND imports the to-be-imported bookmarks, not just preview them.
/// If it is rejected, it can both mean user didn't import anything, or they merely stopped the
///   import operation in the middle.
class ImportedBookmarksPreviewDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::ImportedBookmarksPreviewDialog *ui;
    DatabaseManager* dbm;
    bool canShowTheDialog;
    ImportedEntityList* elist;
    ImportedBookmarksProcessor* m_bookmarksProcessor;

    QStringList tagsForAll;
    QMap<int, QTreeWidgetItem*> folderItems;
    QMap<int, QTreeWidgetItem*> bookmarkItems;
    QMap<int, int> folderItemsIndexInArray;

    QIcon icon_folder;
    QIcon icon_folderdontimport;
    QIcon icon_dontimport;
    QIcon icon_okay;
    QIcon icon_similar;
    QIcon icon_exact;
    QIcon icon_overwrite;
    QIcon icon_append;
    QIcon icon_waiting;
    QIcon icon_fail;

    enum TreeWidgetItemData
    {
        TWID_IsFolder = Qt::UserRole,
        TWID_Index,
    };

public:
    explicit ImportedBookmarksPreviewDialog(DatabaseManager* dbm, BookmarkImporter* bmim,
                                            ImportedEntityList* elist, QWidget *parent = 0);
    ~ImportedBookmarksPreviewDialog();

public:
    bool canShow();

public slots:
    void accept();

private slots:
    void ProcessingDone();
    void ProcessingCanceled();
    void ImportedBookmarkProcessed(ImportedBookmark* ib, bool successful);

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
    void SetBookmarkItemIcon(QTreeWidgetItem* twi, const ImportedBookmark& ib);
    void RecursiveSetFolderImport(QTreeWidgetItem* twi, bool import);
};
