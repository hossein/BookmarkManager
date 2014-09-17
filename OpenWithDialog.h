#pragma once
#include <QDialog>

#include "DatabaseManager.h"

class QToolButton;
class QListWidgetItem;
namespace Ui { class OpenWithDialog; }

/// We want this dialog to be very robust. So we use the same textbox for both filtering the available
/// programs in the list, and if it points to an existent executable file, we will show it as the ONLY
/// item WHETHER it is already in the list or not. If it was not already in the list, then we get its
/// user-friendly title and icon from the real file. But for saved programs, their title is saved in the
/// DB (and user can change it) but the icon is got from the original file each time.
/// User can also delete programs from list, but deleting the programs doesn't change the file types
/// associated with it. There is another place for this.
class OpenWithDialog : public QDialog
{
    Q_OBJECT

public:
    struct OutParams
    {
        //If `selectedSAID == -1` then `browsedSystemAppPath` is also set,
        //  otherwise this path cannot be trusted and used.
        long long selectedSAID;
        QString browsedSystemAppPath;
    };

private:
    Ui::OpenWithDialog *ui;
    DatabaseManager* dbm;
    bool canShowTheDialog;
    OutParams* outParams;

    QToolButton* m_optionsButton;
    QListWidgetItem* m_browsedProgramItem;

public:
    explicit OpenWithDialog(DatabaseManager* dbm, OutParams* outParams = NULL, QWidget *parent = 0);
    ~OpenWithDialog();

public:
    bool canShow();

public slots:
    void accept();

private slots:
    void on_leFilterBrowse_textEdited(const QString& text);
    void on_btnBrowse_clicked();

    void on_lwProgs_itemSelectionChanged();
    void on_lwProgs_itemActivated(QListWidgetItem *item);
    void on_lwProgs_customContextMenuRequested(const QPoint& pos);

    void setProgItemData(QListWidgetItem* item, long long SAID,
                         const QPixmap& pixmap, const QString& text, const QString& path);
    void lwProgsShowAllNonBrowsedItems();
    void lwProgsShowOnlyBrowsedItem();

    void filter();

    void pact_browse();
    void pact_rename();
    void pact_remove();
};
