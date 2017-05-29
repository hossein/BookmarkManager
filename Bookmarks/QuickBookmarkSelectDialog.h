#pragma once
#include <QDialog>

#include "Database/DatabaseManager.h"

namespace Ui { class QuickBookmarkSelectDialog; }

/// This class will NOT return a -1 BID. Check for exec return value to know if accepted.
class QuickBookmarkSelectDialog : public QDialog
{
    Q_OBJECT

public:
    struct OutParams
    {
        QList<long long> selectedBIds;
    };

private:
    Ui::QuickBookmarkSelectDialog *ui;
    DatabaseManager* dbm;
    OutParams* outParams;

public:
    explicit QuickBookmarkSelectDialog(DatabaseManager* dbm, bool scrollToBottom,
                                       OutParams* outParams = NULL, QWidget *parent = 0);
    ~QuickBookmarkSelectDialog();

public slots:
    void accept();

private slots:
    void on_leFilter_textEdited(const QString& text);
    void bvBookmarksActivated(long long BID);
    void bvBookmarksSelectionChanged(const QList<long long>& selectedBIDs);
};
