#include "ImportedBookmarkProcessor.h"

#include "BookmarkImporters/ImportedEntity.h"

#include <QMetaMethod>

ImportedBookmarkProcessor::ImportedBookmarkProcessor(QObject *parent) :
    QObject(parent), m_isProcessing(false)
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

    //TODO: Cancel http requests, etc.

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
    //TODO
}

void ImportedBookmarkProcessor::RetrievePage()
{
    int methodIndex = this->metaObject()->indexOfMethod("PageRetrieved()");
    this->metaObject()->method(methodIndex).invoke(this, Qt::QueuedConnection);
}

void ImportedBookmarkProcessor::PageRetrieved()
{
    m_isProcessing = false;
    emit ImportedBookmarkProcessed(m_currId);
}
