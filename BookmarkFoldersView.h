#pragma once
#include <QWidget>

class BookmarkFoldersTreeWidget;
class QTreeWidgetItem;

class DatabaseManager;

/// The widget that should be used from outside.
class BookmarkFoldersView : public QWidget
{
    Q_OBJECT

private:
    BookmarkFoldersTreeWidget* twFolders;
    DatabaseManager* dbm;
    QAction* m_newAction;
    QAction* m_editAction;
    QAction* m_deleteAction;
    QHash<long long, QTreeWidgetItem*> m_itemForFOID;
    QHash<long long, bool> m_expandedState;
    bool m_onceNoEmitChangeFOID; //But still records last emitted FOID
    long long m_lastEmittedChangeFOID;

public:
    explicit BookmarkFoldersView(QWidget *parent = 0);
    ~BookmarkFoldersView();

    /// This class MUST be initialized after db is ready by calling this function.
    void Initialize(DatabaseManager* dbm);

    long long GetCurrentFOID();
    void SetCurrentFOIDSilently(long long FOID);

    //QWidget interface
protected:
    virtual void focusInEvent(QFocusEvent* event);

private:
    //Select something after calling this function to manage enabled state of toolbar buttons.
    void AddItems(bool rememberExpands);

    void RememberExpands();
    void RestoreExpands();

private slots:
    void twFoldersCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void btnNewFolderClicked();
    void btnEditFolderClicked();
    void btnDeleteFolderClicked();

signals:
    void CurrentFolderChanged(long long FOID);
    void RequestMoveBookmarksToFolder(const QList<long long>& BIDs, long long FOID);

};
