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

    struct QEncodingOptions
    {
        QEncodingOptions()
        {
            QEncoding = false;
            firstLineAdditionalLen = 0;
        }

        bool QEncoding;
        int firstLineAdditionalLen;
        QByteArray originalEncoding;
    };

    static QByteArray EncodeQuotedPrintable(const QByteArray& byteArray, bool binaryData = false,
                                            const QEncodingOptions& qenc = QEncodingOptions());

    static QByteArray DecodeQuotedPrintable(const QByteArray& byteArray);

    //Extract html <title> tag contents. If no <title> tag was found, returns a Null QString.
    static QString ExtractHTMLTitleText(const QString& html);

    //Converts all html entities (e.g "&amp;" to "&") in the string.
    static QString UnEscapeHTMLEntities(const QString& value);

public:
    ///Utility function
    ///`isFileName=true` will not try to find illegal characters for file names, however it
    /// recognizes and re-adds a file extension (only if the extension is less 20 characters).
    /// If transformUnicode is true, the output will contain only ASCII characters.
    static QString SafeAndShortFSName(const QString& fsName, bool isFileName, bool transformUnicode);

    static QString PercentEncodeQChar(const QChar& c);
    static QString PercentEncodeFSChars(const QString& input);

private:
    static QString PercentEncodeUnicodeChars(const QString& input);
    static QString PercentEncodeUnicodeAndFSChars(const QString& input);

public:
    static QString FullyPercentDecodedUrl(const QString& url);

    //String utility functions ////////////////////////////////////////////////////////////////////
    static QString RemoveEmptyLinesAndTrim(const QString& text);

    // StringList Handling ////////////////////////////////////////////////////////////////////////
    static void CaseInsensitiveStringListEliminateDuplicates(QStringList& list);
    static QStringList CaseInsensitiveStringListEliminateDuplicatesCopy(const QStringList& list);

    static void CaseInsensitiveStringListRemoveElement(QStringList& list, const QString& str);

    static void CaseInsensitiveStringListDifference(QStringList& list1, QStringList& list2);

    // Files, Directories, FileSystem /////////////////////////////////////////////////////////////
    ///Can be used to generate a directory name, too.
    static QString NonExistentRandomFileNameInDirectory(const QString& dirPath, int length,
                                                        const QString& prefix = QString(""),
                                                        const QString& extension = QString(""));

    static bool RemoveDirectoryRecursively(const QString& dirPathName, bool removeParentDir = true);

    // File Properties Handling ///////////////////////////////////////////////////////////////////
    static QString UserReadableFileSize(long long size);
    static QByteArray GetMD5HashForFile(const QString& filePathName);
    static bool IsValidFileName(const QString& fileName); //See starting comments

    // Math ///////////////////////////////////////////////////////////////////////////////////////
    static void SeedRandomWithTime();
    static int Random();

    // Database Serialization/Deserialization /////////////////////////////////////////////////////
    static QByteArray SerializeQPixmap(const QPixmap& pixmap);
    static QPixmap DeSerializeQPixmap(const QByteArray& data);

    // Byte Array /////////////////////////////////////////////////////////////////////////////////
    static void FindLineColumnForOffset(const QByteArray& buff, int offset, int& line, int& col);
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

    //For correct set handling, this function requires a `key` function instead of `equals`.
    template <typename T, typename KT>
    static QList<int> ListIntersect(const QList<T>& list1, const QList<T>& list2, KT (*key)(const T& v))
    {
        //Copied from Util::CaseInsensitiveStringListDifference and may be optimized;
        //  read the notes there.

        int len1 = list1.length();
        int len2 = list2.length();

        QList<int> intersectIndexes1;
        QSet<KT> intersectKeys;
        for (int i1 = 0; i1 < len1; i1++)
        {
            KT key1 = key(list1[i1]);
            for (int i2 = 0; i2 < len2; i2++)
            {
                if (key1 == key(list2[i2]))
                {
                    if (!intersectKeys.contains(key1))
                    {
                        intersectIndexes1.append(i1);
                        intersectKeys.insert(key1);
                    }
                }
            }
        }

        return intersectIndexes1;
    }
};
