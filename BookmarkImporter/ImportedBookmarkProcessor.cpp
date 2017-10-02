#include "ImportedBookmarkProcessor.h"

#include "BookmarkImporter.h"
#include "BookmarkImporter/ImportedEntity.h"
#include "MHTSaver.h"
#include "Util/Util.h"

#include <QFileInfo>
#include <QMetaMethod>

ImportedBookmarkProcessor::ImportedBookmarkProcessor(BookmarkImporter* bmim, ImportedEntityList* elist,
                                                     QObject *parent)
    : QObject(parent), m_isProcessing(false), m_bmim(bmim), m_elist(elist), m_ib(NULL)
{
    m_mhtSaver = new MHTSaver(this);
    m_mhtSaver->setOverallTimeoutTime(300); //5 minutes
    connect(m_mhtSaver, SIGNAL(MHTDataReady(QByteArray,MHTSaver::Status)),
            this, SLOT(PageRetrieved(QByteArray,MHTSaver::Status)));
}

ImportedBookmarkProcessor::~ImportedBookmarkProcessor()
{

}

ImportedBookmark*ImportedBookmarkProcessor::lastProcessedImportedBookmark()
{
    return m_ib;
}

bool ImportedBookmarkProcessor::ProcessImportedBookmark(int id, ImportedBookmark* ib)
{
    if (m_isProcessing)
        return false;

    m_isProcessing = true;
    m_currId = id;
    m_ib = ib;

    m_elapsedTimer.start();
    qDebug() << m_currId << " will process " << ib->uri;

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

    if (m_elist->importSource == ImportedEntityList::Source_Urls || m_elist->importSource == ImportedEntityList::Source_Firefox)
        RetrievePage();
    else if (m_elist->importSource == ImportedEntityList::Source_Files)
        Import(); //SetBookmarkTitle() is not needed in this case, title is already a non-empty string
}

void ImportedBookmarkProcessor::EndProcess(bool successful)
{
    m_isProcessing = false;

    qDebug() << m_currId << " Done (" << successful << ") in " << m_elapsedTimer.elapsed() / 1000.0 << " seconds.";

    emit ImportedBookmarkProcessed(m_currId, successful);
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

    if (m_elist->importSource == ImportedEntityList::Source_Urls)
    {
        exInfo.Name = "bm: imported from";
        exInfo.Type = ExInfoData::Type_Text;
        exInfo.Value = "url";
        m_ib->ExPr_ExtraInfosList.append(exInfo);
    }
    else if (m_elist->importSource == ImportedEntityList::Source_Files)
    {
        exInfo.Name = "bm: imported from";
        exInfo.Type = ExInfoData::Type_Text;
        exInfo.Value = "file";
        m_ib->ExPr_ExtraInfosList.append(exInfo);

        exInfo.Name = "file: imported file name";
        exInfo.Type = ExInfoData::Type_Text;
        exInfo.Value = QFileInfo(m_ib->ExMd_importedFilePath).completeBaseName();
        m_ib->ExPr_ExtraInfosList.append(exInfo);
    }
    else if (m_elist->importSource == ImportedEntityList::Source_Firefox)
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

        if (!m_ib->guid.isEmpty())
        {
            exInfo.Name = "firefox: guid";
            exInfo.Type = ExInfoData::Type_Text;
            exInfo.Value = m_ib->guid;
            m_ib->ExPr_ExtraInfosList.append(exInfo);
        }

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

    //Set the file attach error status.
    //In case of success, if HTTP status is not successful errorString will contain the status.
    //In case of both success and HTTP status = 2xx the errorString will be empty.
    m_ib->ExPr_attachedFileName = QString();
    m_ib->ExPr_attachedFileError = QString();

    if (!status.mainSuccess)
    {
        m_ib->ExPr_attachedFileError = QString("Error %1: %2")
                                       .arg(status.mainNetworkReplyError)
                                       .arg(status.mainNetworkReplyErrorString);
        m_ib->ExIm_finalError = m_ib->ExPr_attachedFileError;
        //Mark as error but do NOT end the process, just the file saving failed!
        m_bmim->MarkAsFailed(m_ib);
        //  EndProcess(false);
        //  return;
    }
    if (status.mainSuccess && status.mainHttpErrorCode == 0)
    {
        //User cancelled the operation
        m_ib->ExPr_attachedFileError = QString("User cancelled the web page saving operation.");
        m_ib->ExIm_finalError = m_ib->ExPr_attachedFileError;
        //Do NOT mark as error, just user stopped: m_bmim->MarkAsFailed(m_ib);
        EndProcess(false);
        return;
    }

    //We check the HTTP status here.
    //1xx are informational and should not happen, 2xx show success, 3xx show redirection and
    //  are handled by MHTSaver, 4xx and 5xx are client and server errors.
    //However we consider anything other than 2xx fail!
    if (status.mainHttpErrorCode < 200 || status.mainHttpErrorCode > 299)
    {
        if (!m_ib->ExPr_attachedFileError.isEmpty())
            m_ib->ExPr_attachedFileError += "\n";
        m_ib->ExPr_attachedFileError += QString("HTTP Error %1").arg(status.mainHttpErrorCode);
        m_ib->ExIm_finalError = m_ib->ExPr_attachedFileError;
    }

    //Save the page
    //The file is NOT always an mhtml; MHTSaver does not wrap e.g images and pdfs in this format.
    //ExPr_attachedFileName Will be 'Safe and Short'ed later, but we remove filesystem-unsafe chars.
    m_ib->ExPr_attachedFileName = Util::PercentEncodeFSChars(status.mainResourceTitle);
    if (!status.fileSuffix.isEmpty())
        m_ib->ExPr_attachedFileName += "." + status.fileSuffix;
    m_ib->ExPr_attachedFileData = data;

    //`.simplified` is needed since e.g an i3e explore title contained newlines and tabs in it!
    //Also for e.g pdfs, it won't contain the file extension, but firefox already saved a title for
    //  them so this won't matter.
    SetBookmarkTitle(status.mainResourceTitle.simplified());
}

void ImportedBookmarkProcessor::SetBookmarkTitle(const QString& suggestedTitle)
{
    //It is also important that we set a title for [title-less bookmarks].
    if (m_ib->title.trimmed().isEmpty())
    {
        if (!suggestedTitle.isEmpty())
            m_ib->title = suggestedTitle;
        else
            m_ib->title = Util::FullyPercentDecodedUrl(m_ib->uri);
    }

    Import();
}

void ImportedBookmarkProcessor::Import()
{
    //No need to check return value.
    bool successful = m_bmim->ImportOne(*m_ib, m_elist);

    if (!successful)
    {
        if (!m_ib->ExIm_finalError.isEmpty())
            m_ib->ExIm_finalError += "\n";
        m_ib->ExIm_finalError += "Importer Error.";
        m_bmim->MarkAsFailed(m_ib);
    }

    //After import, free some memory
    m_ib->ExPr_attachedFileData.clear();

    //Next step... done.
    EndProcess(successful);
}
