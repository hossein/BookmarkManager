#pragma once
#include <QString>
#include <QStringList>

class Util
{
public:
    static void CaseInsensitiveStringListEliminateDuplicates(QStringList& list);
    static QStringList CaseInsensitiveStringListEliminateDuplicatesCopy(const QStringList& list);

    static void CaseInsensitiveStringListRemoveElement(QStringList& list, const QString& str);

    static void CaseInsensitiveStringListDifference(QStringList& list1, QStringList& list2);
};
