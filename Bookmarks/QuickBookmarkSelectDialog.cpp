#include "QuickBookmarkSelectDialog.h"
#include "ui_QuickBookmarkSelectDialog.h"

#include <QPushButton>
#include <QScrollBar>

QuickBookmarkSelectDialog::QuickBookmarkSelectDialog(
        DatabaseManager* dbm, bool scrollToBottom, OutParams* outParams, QWidget *parent)
    : QDialog(parent), ui(new Ui::QuickBookmarkSelectDialog), dbm(dbm), outParams(outParams)
{
    ui->setupUi(this);

    //Tags
    ui->leFilter->setModel(&dbm->tags.model);
    ui->leFilter->setModelColumn(dbm->tags.tidx.TagName);

    //BookmarksView
    ui->bvBookmarks->Initialize(dbm, BookmarksView::LM_LimitedDisplayWithHeaders, &dbm->bms.model);
    connect(ui->bvBookmarks, SIGNAL(activated(long long)), this, SLOT(bvBookmarksActivated(long long)));
    connect(ui->bvBookmarks, SIGNAL(currentRowChanged(long long,long long)),
            this, SLOT(bvBookmarksCurrentRowChanged(long long,long long)));

    //Don't enable Ok button unless user selects something.
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    //Scroll to bottom if user likes. E.g good for linking bookmarks.
    if (scrollToBottom)
        ui->bvBookmarks->verticalScrollBar()->setValue(ui->bvBookmarks->verticalScrollBar()->maximum());
}

QuickBookmarkSelectDialog::~QuickBookmarkSelectDialog()
{
    delete ui;
}

void QuickBookmarkSelectDialog::accept()
{
    long long selectedBId = ui->bvBookmarks->GetSelectedBookmarkID();

    if (selectedBId == -1)
        return; //Just in case, and as a requirement of this class.

    if (outParams != NULL)
        outParams->selectedBId = selectedBId;

    QDialog::accept();
}

void QuickBookmarkSelectDialog::on_leFilter_textEdited(const QString& text)
{
    //TODO: Filter by tags, title, url.
}

void QuickBookmarkSelectDialog::bvBookmarksActivated(long long BID)
{
    Q_UNUSED(BID);
    accept();
}

void QuickBookmarkSelectDialog::bvBookmarksCurrentRowChanged(long long currentBID, long long previousBID)
{
    Q_UNUSED(previousBID);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(currentBID != -1);
}
