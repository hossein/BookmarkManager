#include "MergeConfirmationDialog.h"
#include "ui_MergeConfirmationDialog.h"

#include <algorithm>

#include <Util/RichRadioButton.h>

#include "Util/Util.h"

MergeConfirmationDialog::MergeConfirmationDialog(DatabaseManager* dbm, const QList<long long>& BIDs,
                                                 OutParams* outParams, QWidget* parent) :
    QDialog(parent), ui(new Ui::MergeConfirmationDialog), dbm(dbm),
    canShowTheDialog(false), outParams(outParams)
{
    ui->setupUi(this);

    QStringList bookmarkNames;
    QMultiHash<long long, QString> bookmarkURLs;

    canShowTheDialog = dbm->bms.RetrieveBookmarkNames(BIDs, bookmarkNames);
    if (!canShowTheDialog)
        return;

    canShowTheDialog = dbm->bms.RetrieveBookmarkFullURLs(BIDs, bookmarkURLs);
    if (!canShowTheDialog)
        return;

    radioContext = new RichRadioButtonContext();

    for (int i = 0; i < BIDs.count(); i++)
    {
        long long BID = BIDs[i];

        //Create formatted URLs for display.
        //Reverse the urls to get the original order, as QMultiHash docs say:
        //  "The items that share the same key are available from most recently to least recently inserted."
        QStringList urls = bookmarkURLs.values(BID);
        std::reverse(urls.begin(), urls.end());

        QString urlsSeparated = "";
        const QString sep = "";
        foreach (const QString& url, urls)
            urlsSeparated += "<li style=\"color: blue;\">" + Util::FullyPercentDecodedUrl(url).toHtmlEscaped() + "</li>" + sep;
        urlsSeparated.chop(sep.length());

        QString radioText = QString("<strong>%1</strong>\n<ul>%2</ul>").arg(bookmarkNames[i], urlsSeparated);

        RichRadioButton* bmRadio = new RichRadioButton(radioText, BID, (i == 0), radioContext);
        ui->mainVerticalLayout->insertWidget(ui->mainVerticalLayout->count() -2, bmRadio);
    }
}

MergeConfirmationDialog::~MergeConfirmationDialog()
{
    delete radioContext;
    delete ui;
}

bool MergeConfirmationDialog::canShow()
{
    return canShowTheDialog;
}

void MergeConfirmationDialog::accept()
{
    if (!radioContext->value.isValid())
        return; //Can only happen if no item is selected.

    if (outParams != NULL)
    {
        outParams->mainBId = radioContext->value.toLongLong();
    }

    QDialog::accept();
}
