#pragma once
#include <QDialog>

#include "DatabaseManager.h"

namespace Ui { class QuickBookmarkSelectDialog; }

/// This class will NOT return a -1 BID. Check for exec return value to know if accepted.
class QuickBookmarkSelectDialog : public QDialog
{
    Q_OBJECT

public:
    struct OutParams
    {
        long long selectedBId;
    };

private:
    Ui::QuickBookmarkSelectDialog *ui;
    DatabaseManager* dbm;
    Config* conf;
    OutParams* outParams;

public:
    explicit QuickBookmarkSelectDialog(DatabaseManager* dbm, Config* conf, bool scrollToBottom,
                                       OutParams* outParams = NULL, QWidget *parent = 0);
    ~QuickBookmarkSelectDialog();

public slots:
    void accept();

private slots:
    void on_leFilter_textEdited(const QString& text);
    void bvBookmarksActivated(long long BID);
    void bvBookmarksCurrentRowChanged(long long currentBID, long long previousBID);
};
