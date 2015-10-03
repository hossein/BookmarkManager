#pragma once
#include <QObject>

class QProgressDialog;
class BookmarkImporter;
struct ImportedEntityList;
class ImportedBookmarkProcessor;

class ImportedBookmarksProcessor : public QObject
{
    Q_OBJECT

private:
    bool m_isProcessing;
    const int m_processorCount;
    QWidget* m_dialogParent;

    BookmarkImporter* m_bmim;
    ImportedEntityList* m_elist;
    int m_toBeImportedCount;
    int m_processedCount;
    int m_nextProcessIndex;

    QList<ImportedBookmarkProcessor*> m_bookmarkProcessors;
    QProgressDialog* m_progressDialog;

public:
    explicit ImportedBookmarksProcessor(int processorCount, BookmarkImporter* bmim,
                                        QWidget* dialogParent, QObject *parent = 0);

public slots:
    bool BeginProcessing(ImportedEntityList* elist);

private slots:
    void BookmarkProcessed(int id, bool successful);
    void AllBookmarksProcessed();
    void Cancel();

    void CleanUp();

signals:
    void ProcessingDone();
    void ProcessingCanceled();

};
