#pragma once
#include <QDialog>
#include "DatabaseManager.h"
#include "BookmarkImporters/ImportedEntity.h"

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

public:
    explicit ImportedBookmarksPreviewDialog(DatabaseManager* dbm, Config* conf,
                                            ImportedEntityList* elist, QWidget *parent = 0);
    ~ImportedBookmarksPreviewDialog();

public:
    bool canShow();

public slots:
    void accept();
};
