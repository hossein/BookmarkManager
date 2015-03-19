#pragma once
#include <QObject>

class QProgressDialog;
class ImportedEntityList;
class ImportedBookmarkProcessor;

class ImportedBookmarksProcessor : public QObject
{
    Q_OBJECT

private:
    bool m_isProcessing;
    const int m_processorCount;
    QWidget* m_dialogParent;

    ImportedEntityList* m_elist;
    int m_toBeImportedCount;
    int m_processedCount;
    int m_nextProcessIndex;

    ImportedBookmarkProcessor* m_bookmarkProcessors;
    QProgressDialog* m_progressDialog;

public:
    explicit ImportedBookmarksProcessor(int processorCount, QWidget* dialogParent,
                                        ImportedEntityList* elist, QObject *parent = 0);

public slots:
    bool BeginProcessing();

private slots:
    void BookmarkProcessed(int id);
    void ProcessingDone();
    void Cancel();

    void CleanUp();

signals:
    void ProcessingDone();
    void Canceled();

};
