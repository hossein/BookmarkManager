#include "QuickBookmarkSelectDialog.h"
#include "ui_QuickBookmarkSelectDialog.h"

#include "Util/WindowSizeMemory.h"

#include <QPushButton>
#include <QScrollBar>

QuickBookmarkSelectDialog::QuickBookmarkSelectDialog(
        DatabaseManager* dbm, bool scrollToBottom, OutParams* outParams, QWidget *parent)
    : QDialog(parent), ui(new Ui::QuickBookmarkSelectDialog), dbm(dbm), outParams(outParams)
{
    ui->setupUi(this);
    WindowSizeMemory::SetWindowSizeMemory(this, this, dbm, "QuickBookmarkSelectDialog", true, true, false, 1);

    //Tags
    ui->leFilter->setModel(&dbm->tags.model);
    ui->leFilter->setModelColumn(dbm->tags.tidx.TagName);

    //BookmarksView
    ui->bvBookmarks->Initialize(dbm, BookmarksView::LM_LimitedDisplayWithHeaders, &dbm->bms.model);
    connect(ui->bvBookmarks, SIGNAL(activated(long long)), this, SLOT(bvBookmarksActivated(long long)));
    connect(ui->bvBookmarks, SIGNAL(selectionChanged(QList<long long>)),
            this, SLOT(bvBookmarksSelectionChanged(QList<long long>)));

    //Don't enable Ok button unless user selects something.
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    //Scroll to bottom if user likes. E.g good for linking bookmarks.
    if (scrollToBottom)
        ui->bvBookmarks->ScrollToBottom();
}

QuickBookmarkSelectDialog::~QuickBookmarkSelectDialog()
{
    delete ui;
}

void QuickBookmarkSelectDialog::accept()
{
    QList<long long> selectedBIds = ui->bvBookmarks->GetSelectedBookmarkIDs();

    if (selectedBIds.empty())
        return; //Just in case, and as a requirement of this class.

    if (outParams != NULL)
        outParams->selectedBIds = selectedBIds;

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

void QuickBookmarkSelectDialog::bvBookmarksSelectionChanged(const QList<long long>& selectedBIDs)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!selectedBIDs.empty());
}
