#include "ImportedBookmarksProcessor.h"

#include "ImportedBookmarkProcessor.h"
#include "BookmarkImporter/ImportedEntity.h"

#include <QProgressDialog>

ImportedBookmarksProcessor::ImportedBookmarksProcessor(int processorCount, BookmarkImporter* bmim,
                                                       QWidget* dialogParent, QObject *parent)
    : QObject(parent), m_processorCount(processorCount), m_dialogParent(dialogParent), m_bmim(bmim)
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
    m_progressDialog->setWindowTitle("Processing and Importing Bookmarks");
    m_progressDialog->setValue(0);
    m_progressDialog->show();
    connect(m_progressDialog, SIGNAL(canceled()), this, SLOT(Cancel()));

    for (int i = 0; i < m_processorCount; i++)
    {
        ImportedBookmarkProcessor* bookmarkProcessor = new ImportedBookmarkProcessor(m_bmim, m_elist, this);
        connect(bookmarkProcessor, SIGNAL(ImportedBookmarkProcessed(int,bool)),
                this, SLOT(BookmarkProcessed(int,bool)));
        m_bookmarkProcessors.append(bookmarkProcessor);
    }

    if (m_toBeImportedCount == 0)
    {
        AllBookmarksProcessed();
        return true;
    }

    //Add the initial bookmarks for processing.
    m_processedCount = 0;
    m_nextProcessIndex = 0;

    //Wrong: Because we check `m_toBeImportedCount == 0` this doesn't get out of bounds.
    //Right: We have to be careful in the following `while` not to get out of bounds. This is
    //  possible if m_toBeImportedCount is less than m_processorCount. So checking `< size` in the
    //  `while` condition is not enough and we add `initialProcesses < m_toBeImportedCount` too.
    int initialProcesses = 0;
    while (initialProcesses < m_processorCount && initialProcesses < m_toBeImportedCount &&
           m_nextProcessIndex < m_elist->iblist.size())
    {
        while (!m_elist->iblist[m_nextProcessIndex].Ex_finalImport)
            m_nextProcessIndex += 1;

        m_bookmarkProcessors[initialProcesses]->ProcessImportedBookmark(initialProcesses,
                                                                        &(m_elist->iblist[m_nextProcessIndex]));
        initialProcesses += 1;
        m_nextProcessIndex += 1;
    }

    m_isProcessing = true;
    return true;
}

void ImportedBookmarksProcessor::BookmarkProcessed(int id, bool successful)
{
    m_processedCount += 1;
    ImportedBookmark* lastProcessedIB = m_bookmarkProcessors[id]->lastProcessedImportedBookmark();
    QString resultStr = (successful ? "Imported" : "Import was not successful");
    m_progressDialog->setLabelText(QString("%1:<br/>\n<strong>%2</strong><br/>\n(%3)")
                                   .arg(resultStr, lastProcessedIB->title, lastProcessedIB->uri));
    m_progressDialog->setValue(m_processedCount);
    emit ImportedBookmarkProcessed(lastProcessedIB, successful);

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
            m_bookmarkProcessors[id]->ProcessImportedBookmark(id, &(m_elist->iblist[m_nextProcessIndex]));
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
    //Cancelling will emit `ImportedBookmarkProcessed` signals which will in turn cause
    //  'AllBookmarksProcessed` to be emitted, which is both wrong and does a 'CleanUp` before we do
    //  here! We don't want that So disconnect first.
    for (int i = 0; i < m_processorCount; i++)
    {
        disconnect(m_bookmarkProcessors[i], SIGNAL(ImportedBookmarkProcessed(int,bool)), 0, 0);
        m_bookmarkProcessors[i]->Cancel();
    }

    //Since we disabled the signals, we need to clean-up ourselves.
    CleanUp();
    emit ProcessingCanceled();
}

void ImportedBookmarksProcessor::CleanUp()
{
    m_isProcessing = false;

    //Either `hide()` then `delete`, or use `close()` alone.
    m_progressDialog->hide();
    m_progressDialog->deleteLater();

    for (int i = 0; i < m_processorCount; i++)
        m_bookmarkProcessors[i]->deleteLater();
    m_bookmarkProcessors.clear();
}
