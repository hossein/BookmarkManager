#include "TagsView.h"

#include "Database/DatabaseManager.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QScrollBar>

TagsView::TagsView(QWidget* parent) : QWidget(parent), lwTags(NULL)
{
    //Initialize this here to protect from some crashes
    lwTags = new QListWidget(this);
    lwTags->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lwTags->setSelectionMode(QAbstractItemView::SingleSelection);
    lwTags->setSelectionBehavior(QAbstractItemView::SelectRows);
    lwTags->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    lwTags->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    lwTags->setWordWrap(false);
}

void TagsView::Initialize(DatabaseManager* dbm)
{
    //Class members
    this->dbm = dbm;
    this->m_allTagsChecked = TCSR_NoneChecked;

    //UI
    QHBoxLayout* myLayout = new QHBoxLayout();
    myLayout->addWidget(lwTags);
    myLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(myLayout);

    //Connections
    connectTagsChangeSignal();
}

void TagsView::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
    lwTags->setFocus();
}

void TagsView::RefreshUIDataDisplay(bool rePopulateModels,
                                    UIDDRefreshAction refreshAction, long long selectTID,
                                    const QList<long long>& newTIDsToCheck)
{
    //Disconnect this signal-slot connection to make sure ItemChanged
    //  signals for lwTags are not fired while updating the tags view.
    //  We connect it at function's end.
    disconnectTagsChangeSignal();

    int selectedTId = -1;
    int hTagsScrollPos = 0;

    if (refreshAction & RA_SaveSel) //[SavingSelectedBookmarkAndTag]
        selectedTId = GetSelectedTagID();

    if (refreshAction & RA_SaveScrollPos)
        if (lwTags->verticalScrollBar() != NULL)
            hTagsScrollPos = lwTags->verticalScrollBar()->value();
    //ONLY care about check states if there were already tag FILTERations.
    QList<long long> checkedTIDs;
    TagCheckStateResult previousTagsState = m_allTagsChecked;
    if ((refreshAction & RA_SaveCheckState) && (previousTagsState == TCSR_SomeChecked))
        checkedTIDs  = GetCheckedTIDs();

    if (rePopulateModels)
        dbm->tags.PopulateModelsAndInternalTables();

    if (!(refreshAction & RA_NoRefreshView))
        RefreshTagsDisplay();

    //Now make sure those we want to check are checked.
    if (refreshAction & RA_SaveCheckState)
    {
        //If either None or Some or All tags are checked, we need to preserve their check state.
        //  Also if a new tag is added in this situation, it needs to become Checked so that it's
        //  visible in the tags list, using newTIDsToCheck.
        if (previousTagsState == TCSR_NoneChecked)
        {
            //Leave them unchecked
            //No need to CheckAllTags(checkState); Just update our variable:
            m_allTagsChecked = TCSR_NoneChecked;
        }
        else if (previousTagsState == TCSR_SomeChecked)
        {
            //Only check these new things if our list was already filtered by tags.
            //New TIDs are already added to `tagItems` as the tags are refreshed above.
            //  So it's safe that the following function references `tagItems[newCheckedTID]`.
            //  Also we are checking additional items ONLY IF RA_SaveCheckState is set.
            RestoreCheckedTIDs(checkedTIDs, newTIDsToCheck);
        }
        else if (previousTagsState == TCSR_AllChecked)
        {
            CheckAllTags(Qt::Checked);
        }
    }

    if (refreshAction & RA_SaveSel)
        if (selectedTId != -1)
            SelectTagWithID(selectedTId);

    if (refreshAction & RA_SaveScrollPos)
        if (lwTags->verticalScrollBar() != NULL)
            lwTags->verticalScrollBar()->setValue(hTagsScrollPos);

    //[RestoringScrollPositionProceedsCustomSelection]
    if (refreshAction & RA_CustomSelect)
        if (selectTID != -1)
            SelectTagWithID(selectTID);

    //Focusing comes last anyway
    if (refreshAction & RA_Focus)
        lwTags->setFocus();

    //This is important, without this, if RA_SaveSel is not used, value of `m_allTagsChecked` remains wrong.
    UpdateAllTagsCheckedStatus();

    connectTagsChangeSignal();
}

bool TagsView::areAllTagsChecked()
{
    //Note that we return true also if no tags are checked; it's like all of them being checked.
    return (m_allTagsChecked == TCSR_AllChecked || m_allTagsChecked == TCSR_NoneChecked);
}

QList<long long> TagsView::GetCheckedTIDs()
{
    QList<long long> checkedTIDs;
    for (auto it = tagItems.constBegin(); it != tagItems.constEnd(); ++it)
        if (it.value()->checkState() == Qt::Checked) //"All Items" does not appear in `tagItems`.
            checkedTIDs.append(it.key());
    return checkedTIDs;
}

QString TagsView::GetCheckedTagsNames()
{
    QString checkedTagsNames;
    foreach (QListWidgetItem* tagItem, tagItems) //Values
        if (tagItem->checkState() == Qt::Checked)
            checkedTagsNames += tagItem->text() + ", ";
    //We are sure there was at least one tag selected.
    checkedTagsNames.chop(2);
    return checkedTagsNames;
}

void TagsView::connectTagsChangeSignal()
{
    connect(lwTags, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
}

void TagsView::disconnectTagsChangeSignal()
{
    //Strange: This doesn't work:
    //disconnect(lwTags, SIGNAL(itemChanged(QListWidgetItem*)));
    lwTags->disconnect(SIGNAL(itemChanged(QListWidgetItem*)));
}

void TagsView::RefreshTagsDisplay()
{
    //Explains checkboxes with both QListWidget and QListView.
    //  http://www.qtcentre.org/threads/47119-checkbox-on-QListView

    QListWidgetItem* item;
    long long theTID;

    tagItems.clear();
    lwTags->clear();

    //Add the bold "All Tags" item at top which get checked/unchecked when an item is
    //  selected/deselected.
    //We don't need to keep the "All Tags" item separate.
    //  We simply don't put it in `tagItems` to not require skipping the first item or something.
    item = new QListWidgetItem("All Tags");
    QFont boldFont(this->font());
    boldFont.setBold(true);
    item->setFont(boldFont);
    item->setCheckState(Qt::Unchecked);
    item->setData(Qt::UserRole + 0, -1);
    lwTags->addItem(item);

    //Add the rest of the tags.
    const int TIDIdx = dbm->tags.tidx.TID;
    const int tagNameIdx = dbm->tags.tidx.TagName;
    int tagsCount = dbm->tags.model.rowCount();
    for (int i = 0; i < tagsCount; i++)
    {
        item = new QListWidgetItem(dbm->tags.model.data(dbm->tags.model.index(i, tagNameIdx)).toString());
        item->setCheckState(Qt::Unchecked);
        lwTags->addItem(item);

        theTID = dbm->tags.model.data(dbm->tags.model.index(i, TIDIdx)).toLongLong();
        item->setData(Qt::UserRole + 0, theTID);
        tagItems[theTID] = item;
    }
}

long long TagsView::GetSelectedTagID()
{
    if (lwTags->selectedItems().size() == 0)
        return -1;
    return lwTags->selectedItems()[0]->data(Qt::UserRole + 0).toLongLong();
}

void TagsView::SelectTagWithID(long long tagId)
{
    if (!tagItems.contains(tagId))
        return;

    QListWidgetItem* item = tagItems[tagId];

    lwTags->selectedItems().clear();
    lwTags->selectedItems().append(item);
    lwTags->scrollToItem(item, QAbstractItemView::EnsureVisible);
}

void TagsView::UpdateAllTagsCheckedStatus()
{
    //See if all items are checked/unchecked.
    bool seenChecked = false;
    bool seenUnchecked = false;
    foreach (QListWidgetItem* item, tagItems.values())
    {
        if (seenChecked && seenUnchecked)
            break;

        if (item->checkState() == Qt::Checked)
            seenChecked = true;
        else if (item->checkState() == Qt::Unchecked)
            seenUnchecked = true;
    }

    //Set the correct `m_allTagsChecked` value.
    if (seenChecked && seenUnchecked)
        m_allTagsChecked = TCSR_SomeChecked;
    else if (seenChecked) // && !seenUnchecked
        m_allTagsChecked = TCSR_AllChecked;
    else if (seenUnchecked) // && !seenChecked
        m_allTagsChecked = TCSR_NoneChecked;
    else //Empty tag items.
        m_allTagsChecked = TCSR_NoneChecked;

    //Update the 'All Tags' checkbox check.
    if (m_allTagsChecked == TCSR_NoneChecked)
        lwTags->item(0)->setCheckState(Qt::Unchecked);
    else if (m_allTagsChecked == TCSR_SomeChecked)
        lwTags->item(0)->setCheckState(Qt::PartiallyChecked);
    else if (m_allTagsChecked == TCSR_AllChecked)
        lwTags->item(0)->setCheckState(Qt::Checked);
}

void TagsView::CheckAllTags(Qt::CheckState checkState)
{
    //First do the `tagItems`.
    foreach (QListWidgetItem* item, tagItems.values())
        item->setCheckState(checkState);

    //IMPORTANT: Update our variable.
    //Note: It was standard to call the `UpdateAllTagsCheckedStatus();` function here, but that
    //  meant an additional `for` loop. So we do it directly here.
    m_allTagsChecked = (checkState == Qt::Checked ? TCSR_AllChecked : TCSR_NoneChecked);

    //Additionally set the check state for 'All Tags', too; although this function is also called
    //  from its event handler and we don't need it in that case.
    lwTags->item(0)->setCheckState(checkState);
}

void TagsView::RestoreCheckedTIDs(const QList<long long>& checkedTIDs, const QList<long long>& newTIDsToCheck)
{
    foreach (long long checkTID, checkedTIDs)
        tagItems[checkTID]->setCheckState(Qt::Checked);

    foreach (long long checkTID, newTIDsToCheck)
        tagItems[checkTID]->setCheckState(Qt::Checked);

    UpdateAllTagsCheckedStatus();
}

void TagsView::itemChanged(QListWidgetItem* item)
{
    //Disconnect this signal-slot connection to make sure ItemChanged
    //  signals for lwTags are not fired while updating the tag item's checkState.
    //  We connect it at function's end.
    //static int i = 0;
    //qDebug() << "CHANGE" << i++;
    disconnectTagsChangeSignal();

    long long theTID = item->data(Qt::UserRole + 0).toLongLong();

    if (theTID == -1)
    {
        //The "All Tags" item was checked/unchecked. Check/Uncheck all tags.
        Qt::CheckState checkState = item->checkState();
        CheckAllTags(checkState);
    }
    else
    {
        //A tag item was checked/unchecked. Set `m_allTagsChecked` and "All Tags" check state.
        UpdateAllTagsCheckedStatus();
    }

    //Connect the signal we disconnected.
    connectTagsChangeSignal();

    emit tagSelectionChanged();
}
