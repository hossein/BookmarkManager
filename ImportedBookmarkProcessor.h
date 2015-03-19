#pragma once
#include <QObject>

class ImportedBookmark;

/// Adds any extra information to the bookmark, as well as getting the HTML page or possibly
/// websnap of the pages. Works asynchronously.
class ImportedBookmarkProcessor : public QObject
{
    Q_OBJECT

private:
    bool m_isProcessing;
    int m_currId;
    ImportedBookmark* m_ib;

public:
    explicit ImportedBookmarkProcessor(QObject *parent = 0);

public slots:
    bool ProcessImportedBookmark(int id, ImportedBookmark* ib);
    bool Cancel();

signals:
    void ImportedBookmarkProcessed(int id);

private slots:
    void BeginProcess();

    void AddMetaData();
    void RetrievePage();
    void PageRetrieved();

};
