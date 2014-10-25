#pragma once
#include <QDebug>
#include <QByteArray>
#include <QPixmap>
#include <QString>
#include <QStringList>

class Util
{
public:
    // Strings ////////////////////////////////////////////////////////////////////////////////////
    static QString RandomHash(int length);

    // StringList Handling ////////////////////////////////////////////////////////////////////////
    static void CaseInsensitiveStringListEliminateDuplicates(QStringList& list);
    static QStringList CaseInsensitiveStringListEliminateDuplicatesCopy(const QStringList& list);

    static void CaseInsensitiveStringListRemoveElement(QStringList& list, const QString& str);

    static void CaseInsensitiveStringListDifference(QStringList& list1, QStringList& list2);

    // Files, Directories, FileSystem /////////////////////////////////////////////////////////////
    static QString NonExistentRandomFileNameInDirectory(const QString& dirPath, int length,
                                                        const QString& prefix = QString(""),
                                                        const QString& extension = QString(""));

    static bool RemoveDirectoryRecursively(const QString& dirPathName, bool removeParentDir = true);

    // File Properties Handling ///////////////////////////////////////////////////////////////////
    static QString UserReadableFileSize(long long size);
    static QByteArray GetMD5HashForFile(const QString& filePathName);
    static bool IsValidFileName(const QString& fileName);

    // Math ///////////////////////////////////////////////////////////////////////////////////////
    static void SeedRandomWithTime();
    static int Random();

    // Database Serialization/Deserialization /////////////////////////////////////////////////////
    static QByteArray SerializeQPixmap(const QPixmap& pixmap);
    static QPixmap DeSerializeQPixmap(const QByteArray& data);
};

class UtilT
{
public:
    // List Handling //////////////////////////////////////////////////////////////////////////////
    template <typename T>
    static void ListDifference(QList<T>& list1, QList<T>& list2)
    {
        //Copied from Util::CaseInsensitiveStringListDifference and may be optimized;
        //  read the notes there.

        int len1 = list1.length();
        int len2 = list2.length();

        QSet<int> removeSet1, removeSet2;
        for (int i1 = 0; i1 < len1; i1++)
        {
            for (int i2 = 0; i2 < len2; i2++)
            {
                if (list2[i2] == list1[i1])
                {
                    removeSet1.insert(i1);
                    removeSet2.insert(i2);
                }
            }
        }

        QList<int> removeList = removeSet1.toList();
        qSort(removeList.begin(), removeList.end());
        for (int r = removeList.size() - 1; r >= 0; r--)
            list1.removeAt(removeList[r]);

        removeList = removeSet2.toList();
        qSort(removeList.begin(), removeList.end());
        for (int r = removeList.size() - 1; r >= 0; r--)
            list2.removeAt(removeList[r]);
    }

    //Decided not to generalize this function, e.g typename EqualsFunc. http://stackoverflow.com/a/5853295/656366.
    template <typename T>
    static void ListDifference(QList<T>& list1, QList<T>& list2, bool (*equals)(const T& v1, const T& v2))
    {
        //Copied from Util::CaseInsensitiveStringListDifference and may be optimized;
        //  read the notes there.

        int len1 = list1.length();
        int len2 = list2.length();

        QSet<int> removeSet1, removeSet2;
        for (int i1 = 0; i1 < len1; i1++)
        {
            for (int i2 = 0; i2 < len2; i2++)
            {
                if (equals(list1[i1], list2[i2]))
                {
                    removeSet1.insert(i1);
                    removeSet2.insert(i2);
                }
            }
        }

        QList<int> removeList = removeSet1.toList();
        qSort(removeList.begin(), removeList.end());
        for (int r = removeList.size() - 1; r >= 0; r--)
            list1.removeAt(removeList[r]);

        removeList = removeSet2.toList();
        qSort(removeList.begin(), removeList.end());
        for (int r = removeList.size() - 1; r >= 0; r--)
            list2.removeAt(removeList[r]);
    }
};
