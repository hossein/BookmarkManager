#include "ImportedBookmarksProcessor.h"

#include "ImportedBookmarkProcessor.h"
#include "BookmarkImporters/ImportedEntity.h"

#include <QProgressDialog>

ImportedBookmarksProcessor::ImportedBookmarksProcessor(int processorCount, QWidget* dialogParent, QObject *parent) :
    QObject(parent), m_processorCount(processorCount), m_dialogParent(dialogParent)
{
    m_isProcessing = false;
}

bool ImportedBookmarksProcessor::BeginProcessing(ImportedEntityList* elist)
{
    if (m_isProcessing)
        return false;

    m_elist = elist;

    m_toBeImportedCount = 0;
    foreach (const ImportedBookmark& ib, elist->iblist)
        if (ib.Ex_finalImport)
            m_toBeImportedCount += 1;

    m_progressDialog = new QProgressDialog("Processing bookmarks, please wait...", "Cancel", 0, m_toBeImportedCount, m_dialogParent);
    m_progressDialog->setValue(0);
    m_progressDialog->show();
    connect(m_progressDialog, SIGNAL(canceled()), this, SLOT(Cancel()));

    m_bookmarkProcessors = new ImportedBookmarkProcessor[m_processorCount];
    for (int i = 0; i < m_processorCount; i++)
    {
        m_bookmarkProcessors[i].setParent(this);
        m_bookmarkProcessors[i].setImportedEntityList(elist);
        connect(m_bookmarkProcessors, SIGNAL(ImportedBookmarkProcessed(int)), this, SLOT(BookmarkProcessed(int)));
    }

    if (m_toBeImportedCount == 0)
    {
        AllBookmarksProcessed();
        return true;
    }

    //Add the initial bookmarks for processing.
    m_processedCount = 0;
    m_nextProcessIndex = 0;

    int initialProcesses = 0;
    while (initialProcesses < m_processorCount && m_nextProcessIndex < m_elist->iblist.size())
    {
        while (!m_elist->iblist[m_nextProcessIndex].Ex_finalImport)
            m_nextProcessIndex += 1;

        m_bookmarkProcessors[initialProcesses].ProcessImportedBookmark(initialProcesses,
                                                                       &(m_elist->iblist[m_nextProcessIndex]));
        initialProcesses += 1;
        m_nextProcessIndex += 1;
    }

    m_isProcessing = true;
    return true;
}

void ImportedBookmarksProcessor::BookmarkProcessed(int id)
{
    m_processedCount += 1;
    m_progressDialog->setValue(m_processedCount);

    if (m_processedCount == m_toBeImportedCount) //Last processor
    {
        AllBookmarksProcessed();
    }
    else
    {
        //Add new bookmarks to process, or just wait for other processors to finish.
        int ibsize = m_elist->iblist.size();

        //This bound check should not be needed as we know we still have bookmarks to process.
        //But this way we are 'more sure'.
        while (m_nextProcessIndex < ibsize && !m_elist->iblist[m_nextProcessIndex].Ex_finalImport)
            m_nextProcessIndex += 1;

        if (m_nextProcessIndex < ibsize)
        {
            m_bookmarkProcessors[id].ProcessImportedBookmark(id, &(m_elist->iblist[m_nextProcessIndex]));
            m_nextProcessIndex += 1;
        }
    }
}

void ImportedBookmarksProcessor::AllBookmarksProcessed()
{
    CleanUp();
    emit ProcessingDone();
}

void ImportedBookmarksProcessor::Cancel()
{
    for (int i = 0; i < m_processorCount; i++)
        m_bookmarkProcessors[i].Cancel();

    CleanUp();
    emit ProcessingCanceled();
}

void ImportedBookmarksProcessor::CleanUp()
{
    m_isProcessing = false;
    //Either `hide()` then `delete`, or use `close()` alone.
    m_progressDialog->hide();
    delete m_progressDialog;
    delete[] m_bookmarkProcessors;
}
