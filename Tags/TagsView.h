#pragma once
#include <QWidget>

#include "Config.h"

class QListWidget;
class QListWidgetItem;

class DatabaseManager;

/// To make this class work, caller needs to create an instance AND call `Initialize`.
class TagsView : public QWidget
{
    Q_OBJECT

private:
    QListWidget* lwTags;
    DatabaseManager* dbm;
    QHash<long long, QListWidgetItem*> tagItems;

    enum TagCheckStateResult
    {
        TCSR_NoneChecked = 0,
        TCSR_SomeChecked = 1,
        TCSR_AllChecked = 2,
    };
    TagCheckStateResult m_allTagsChecked;

public:
    explicit TagsView(QWidget *parent = 0);

    /// This class MUST be initialized after db is ready by calling this function.
    void Initialize(DatabaseManager* dbm);

    //QWidget interface
protected:
    virtual void focusInEvent(QFocusEvent* event);

    //Action and Information Functions
public:
    void RefreshUIDataDisplay(bool rePopulateModels,
                              UIDDRefreshAction refreshAction = RA_None, long long selectTID = -1,
                              const QList<long long>& newTIDsToCheck = QList<long long>());

    bool areAllTagsChecked();
    QList<long long> GetCheckedTIDs();
    QString GetCheckedTagsNames();

private:
    void connectTagsChangeSignal();
    void disconnectTagsChangeSignal();

    void RefreshTagsDisplay();
    long long GetSelectedTagID();
    void SelectTagWithID(long long tagId);

    //Updates both `m_allTagsChecked` and the 'All Tags' checkbox. Must be called after each change.
    void UpdateAllTagsCheckedStatus();

    //The following function automatically Queries and updates checkbox, too.
    void CheckAllTags(Qt::CheckState checkState);
    void RestoreCheckedTIDs(const QList<long long>& checkedTIDs,
                            const QList<long long>& newTIDsToCheck);

private slots:
     void itemChanged(QListWidgetItem* item);

signals:
    void tagSelectionChanged();
};
