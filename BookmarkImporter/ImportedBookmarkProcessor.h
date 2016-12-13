#pragma once
#include <QObject>

#include "MHTSaver.h"

#include <QElapsedTimer>

class BookmarkImporter;
struct ImportedBookmark;
struct ImportedEntityList;

/// Adds any extra information to the bookmark, as well as getting the HTML page or possibly
/// websnap of the pages. Works asynchronously.
class ImportedBookmarkProcessor : public QObject
{
    Q_OBJECT

private:
    bool m_isProcessing;
    int m_currId;
    BookmarkImporter* m_bmim;
    ImportedEntityList* m_elist;
    ImportedBookmark* m_ib;
    MHTSaver* m_mhtSaver;
    QElapsedTimer m_elapsedTimer;

public:
    explicit ImportedBookmarkProcessor(BookmarkImporter* bmim, ImportedEntityList* elist,
                                       QObject *parent = 0);
    ~ImportedBookmarkProcessor();

    ImportedBookmark* lastProcessedImportedBookmark();

public slots:
    bool ProcessImportedBookmark(int id, ImportedBookmark* ib);
    bool Cancel(); ///After Cancel, `ImportedBookmarkProcessed` will be emitted

signals:
    void ImportedBookmarkProcessed(int id, bool successful);

private slots:
    void BeginProcess();
    void EndProcess(bool successful);

    void AddMetaData();
    void RetrievePage();
    void PageRetrieved(const QByteArray& data, const MHTSaver::Status& status);
    void SetBookmarkTitle(const QString& suggestedTitle);

    void Import();
};
