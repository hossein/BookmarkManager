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

    // List Handling //////////////////////////////////////////////////////////////////////////////
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
