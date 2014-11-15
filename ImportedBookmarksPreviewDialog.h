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
    //QMap<int, ImportedBookmarkFolder*> importedFolders;
    //QMap<int, ImportedBookmarkFolder*> importedFolders;

public:
    explicit ImportedBookmarksPreviewDialog(DatabaseManager* dbm, Config* conf,
                                            ImportedEntityList* elist, QWidget *parent = 0);
    ~ImportedBookmarksPreviewDialog();

public:
    bool canShow();

public slots:
    void accept();

private:
    void AddItems();
};
