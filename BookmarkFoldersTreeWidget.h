#pragma once
#include <QTreeWidget>

class Config;

/// Subclassed to receive drop signals. This is just to be used in BookmarkFoldersView.
class BookmarkFoldersTreeWidget : public QTreeWidget
{
    Q_OBJECT

private:
    Config* conf;

public:
    explicit BookmarkFoldersTreeWidget(QWidget *parent = NULL);
    ~BookmarkFoldersTreeWidget();

    /// This class MUST be initialized by calling this function.
    void Initialize(Config* conf);

private:
    bool dropResultsInValidIndex(const QPoint& pos);

    // QWidget interface
protected:
    /// Reimplementing `dragMoveEvent` was unneeded if Qt implemented showing the 'stop' cursor on
    /// invalid areas.
    virtual void dragMoveEvent(QDragMoveEvent* event);
    /// Reimplementing `dropEvent` was unneeded if Qt implemented specifying dragging mouse buttons
    /// on dragStart item view.
    virtual void dropEvent(QDropEvent* event);

    // QTreeWidget interface
protected:
    virtual bool dropMimeData(QTreeWidgetItem* parent, int index, const QMimeData* data, Qt::DropAction action);
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;

signals:
    void RequestMoveBookmarksToFolder(const QList<long long>& BIDs, long long FOID);
};
