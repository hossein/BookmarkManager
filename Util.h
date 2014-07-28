#pragma once
#include <QString>
#include <QStringList>

class Util
{
public:
    // List Handling //////////////////////////////////////////////////////////////////////////////
    static void CaseInsensitiveStringListEliminateDuplicates(QStringList& list);
    static QStringList CaseInsensitiveStringListEliminateDuplicatesCopy(const QStringList& list);

    static void CaseInsensitiveStringListRemoveElement(QStringList& list, const QString& str);

    static void CaseInsensitiveStringListDifference(QStringList& list1, QStringList& list2);

    // File Properties Handling ///////////////////////////////////////////////////////////////////
    static QString UserReadableFileSize(long long size);
    static QByteArray GetMD5HashForFile(const QString& fileName);

    // Math ///////////////////////////////////////////////////////////////////////////////////////
    static void SeedRandomWithTime();
    static int Random();
};
