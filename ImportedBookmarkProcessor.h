#pragma once
#include <QObject>

#include "MHTSaver.h"

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
    ImportedEntityList* m_elist;
    ImportedBookmark* m_ib;
    MHTSaver* m_mhtSaver;

public:
    explicit ImportedBookmarkProcessor(QObject *parent = 0);
    void setImportedEntityList(ImportedEntityList* ielist);
    ~ImportedBookmarkProcessor();

public slots:
    bool ProcessImportedBookmark(int id, ImportedBookmark* ib);
    bool Cancel(); ///After Cancel, `ImportedBookmarkProcessed` will be emitted

signals:
    void ImportedBookmarkProcessed(int id);

private slots:
    void BeginProcess();

    void AddMetaData();
    void RetrievePage();
    void PageRetrieved(const QByteArray& data, const MHTSaver::Status& status);

};
