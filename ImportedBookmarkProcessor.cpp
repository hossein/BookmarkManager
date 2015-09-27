#include "ImportedBookmarkProcessor.h"

#include "BookmarkImporters/ImportedEntity.h"
#include "MHTSaver.h"
#include "Util.h"

#include <QMetaMethod>

ImportedBookmarkProcessor::ImportedBookmarkProcessor(QObject *parent) :
    QObject(parent), m_isProcessing(false), m_mhtSaver()
{
    connect(&m_mhtSaver, SIGNAL(MHTDataReady(QByteArray,MHTSaver::Status)),
            this, SLOT(PageRetrieved(QByteArray,MHTSaver::Status)));
}

bool ImportedBookmarkProcessor::ProcessImportedBookmark(int id, ImportedBookmark* ib)
{
    if (m_isProcessing)
        return false;

    m_isProcessing = true;
    m_currId = id;
    m_ib = ib;

    int methodIndex = this->metaObject()->indexOfMethod("BeginProcess()");
    this->metaObject()->method(methodIndex).invoke(this, Qt::QueuedConnection);

    return true;
}

bool ImportedBookmarkProcessor::Cancel()
{
    if (!m_isProcessing)
        return false;

    m_mhtSaver.Cancel();

    m_isProcessing = false;
    return true;
}

void ImportedBookmarkProcessor::BeginProcess()
{
    AddMetaData();
    RetrievePage();
}

void ImportedBookmarkProcessor::AddMetaData()
{
    //TODO: add extra attributes, e.g firefox's date added and the real import date and 'imported from: firefox', and 'fxprofilename: 2nvgyxqez'
    //          for easy filtering in the future.
}

void ImportedBookmarkProcessor::RetrievePage()
{
    m_mhtSaver.GetMHTData(m_ib->uri);
    int methodIndex = this->metaObject()->indexOfMethod("PageRetrieved()");
    this->metaObject()->method(methodIndex).invoke(this, Qt::QueuedConnection);
}

void ImportedBookmarkProcessor::PageRetrieved(const QByteArray& data, const MHTSaver::Status& status)
{
    if (!m_isProcessing) //i.e MHTSaver's status.mainSuccess = false and status.mainHttpErrorCode = 0.
        return;

    QString errorString = QString();

    if (status.mainSuccess)
    {
        //We check the HTTP status here.
        //1xx are informational and should not happen, 2xx show success, 3xx show redirection and
        //  are handled by MHTSaver, 4xx and 5xx are client and server errors.
        //However we consider anything other than 2xx fail!
        if (status.mainHttpErrorCode < 200 || status.mainHttpErrorCode > 299)
            errorString = QString("HTTP Error %1").arg(status.mainHttpErrorCode);

        //Save the page
        //We know the file is ALWAYS an mhtml, because MHTSaver even wraps images in this format.
        m_ib->ExPr_attachedFileName =
                Util::PercentEncodeUnicodeAndFSChars(status.mainResourceTitle) + ".mhtml";
        m_ib->ExPr_attachedFileData = data;
    }
    else
    {
        errorString = QString("Error %1: %2").arg(status.mainNetworkReplyError)
                                             .arg(status.mainNetworkReplyErrorString);
    }

    m_isProcessing = false;

    //Set the file attach error status.
    //In case of success, if HTTP status is not successful errorString will contain the status.
    //In case of both success and HTTP status = 2xx the errorString will be empty.
    m_ib->ExPr_attachedFileError = errorString;

    emit ImportedBookmarkProcessed(m_currId);
}
