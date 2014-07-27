#include "Util.h"

void Util::CaseInsensitiveStringListEliminateDuplicates(QStringList& list)
{
    QStringList temp(list);
    list.clear();
    for (int i = 0; i < temp.count(); i++)
        if (!list.contains(temp.at(i), Qt::CaseInsensitive))
            list.append(temp.at(i));
}

QStringList Util::CaseInsensitiveStringListEliminateDuplicatesCopy(const QStringList& list)
{
    QStringList nonduplicates;
    for (int i = 0; i < list.count(); i++)
        if (!nonduplicates.contains(list.at(i), Qt::CaseInsensitive))
            nonduplicates.append(list.at(i));
    return nonduplicates;
}

void Util::CaseInsensitiveStringListRemoveElement(QStringList& list, const QString& str)
{
    //Since QStringList::remove is case-Sensitive, we roll our own algorithm for removing.
    //Could use a single `removeAt` in the `for` if we know list doesn't contain duplicates.
    QStringList tempList(list);
    list.clear();
    for (int i = 0; i < tempList.count(); i++)
        if (tempList.at(i).compare(str, Qt::CaseInsensitive) != 0)
            list.append(tempList.at(i));
}

void Util::CaseInsensitiveStringListDifference(QStringList& list1, QStringList& list2)
{
    //TODO: Needed?
}
