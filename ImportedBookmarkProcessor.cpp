#include "ImportedBookmarkProcessor.h"

#include "BookmarkImporters/ImportedEntity.h"
#include "MHTSaver.h"
#include "Util.h"

#include <QMetaMethod>

ImportedBookmarkProcessor::ImportedBookmarkProcessor(QObject *parent) :
    QObject(parent), m_isProcessing(false), m_elist(NULL)
{
    m_mhtSaver = new MHTSaver(this);
    m_mhtSaver->setOverallTimeoutTime(300); //5 minutes
    connect(m_mhtSaver, SIGNAL(MHTDataReady(QByteArray,MHTSaver::Status)),
            this, SLOT(PageRetrieved(QByteArray,MHTSaver::Status)));
}

void ImportedBookmarkProcessor::setImportedEntityList(ImportedEntityList* elist)
{
    m_elist = elist;
}

ImportedBookmarkProcessor::~ImportedBookmarkProcessor()
{

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

    m_mhtSaver->Cancel();

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
    typedef BookmarkManager::BookmarkExtraInfoData ExInfoData;
    ExInfoData exInfo;

    exInfo.Name = "bm: imported";
    exInfo.Type = ExInfoData::Type_Boolean;
    exInfo.Value = "true";
    m_ib->ExPr_ExtraInfosList.append(exInfo);

    if (m_elist == NULL)
        return; //Caller should have set it.

    if (m_elist->importSource == ImportedEntityList::Source_Firefox)
    {
        exInfo.Name = "bm: imported from";
        exInfo.Type = ExInfoData::Type_Text;
        exInfo.Value = "firefox";
        m_ib->ExPr_ExtraInfosList.append(exInfo);

        if (!m_elist->importSourceProfile.isEmpty())
        {
            exInfo.Name = "firefox: profile name";
            exInfo.Type = ExInfoData::Type_Text;
            exInfo.Value = m_elist->importSourceProfile;
            m_ib->ExPr_ExtraInfosList.append(exInfo);
        }

        if (!m_elist->importSourceFileName.isEmpty())
        {
            exInfo.Name = "firefox: json file";
            exInfo.Type = ExInfoData::Type_Text;
            exInfo.Value = m_elist->importSourceFileName;
            m_ib->ExPr_ExtraInfosList.append(exInfo);
        }

        exInfo.Name = "firefox: guid";
        exInfo.Type = ExInfoData::Type_Text;
        exInfo.Value = m_ib->guid;
        m_ib->ExPr_ExtraInfosList.append(exInfo);

        exInfo.Name = "firefox: charset";
        exInfo.Type = ExInfoData::Type_Text;
        exInfo.Value = m_ib->charset;
        m_ib->ExPr_ExtraInfosList.append(exInfo);

        exInfo.Name = "firefox: date added";
        exInfo.Type = ExInfoData::Type_Number;
        exInfo.Value = QString::number(m_ib->dtAdded.toMSecsSinceEpoch());
        m_ib->ExPr_ExtraInfosList.append(exInfo);

        exInfo.Name = "firefox: date modified";
        exInfo.Type = ExInfoData::Type_Number;
        exInfo.Value = QString::number(m_ib->dtModified.toMSecsSinceEpoch());
        m_ib->ExPr_ExtraInfosList.append(exInfo);
    }
}

void ImportedBookmarkProcessor::RetrievePage()
{
    m_mhtSaver->GetMHTData(m_ib->uri);
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
        //The file is NOT always an mhtml; MHTSaver does not wrap e.g images and pdfs in this format.
        //If the file is not mhtml, the status.fileSuffix will not be empty and the title ALREADY
        //  contains the full file name.
        m_ib->ExPr_attachedFileName = Util::PercentEncodeUnicodeAndFSChars(status.mainResourceTitle);
        if (status.fileSuffix.isEmpty()) //The file is an mhtml
            m_ib->ExPr_attachedFileName += ".mhtml";
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
