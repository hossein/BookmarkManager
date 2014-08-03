#include "Util.h"

#include <cstdlib> //rand, srand
#include <ctime> //time

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

QString Util::RandomHash(int length)
{
    char randomHashChars[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
    };
    int randomHashCharsSize = sizeof(randomHashChars) / sizeof(char);

    Util::SeedRandomWithTime(); //We do it for each file we are adding.

    QString randomHash = "";
    for (int i = 0; i < length; i++)
    {
        int charIdx = Util::Random() % randomHashCharsSize;
        randomHash += randomHashChars[charIdx];
    }

    return randomHash;
}

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
    //If we call `CaseInsensitiveStringListEliminateDuplicates` for both lists at the beginning,
    //  the conditions inside the for loop are not needed, however removing the duplicates can be
    //  time consuming, so we prefer the "conditional" way.
    //Note: One might think of using `QSet`s instead of QList's for the to-be-removed's.
    //  BUT They don't preserve order, we need descending order for removing the elements!
    //  The docs say we can use QMap instead, but I am lazy and don't implement it now, because
    //  that needs reverse iterating with QMapIterator or sth that we don't implement because I
    //  don't know of speed impacts of this (QMap::keys, etc don't return the keys in order).
    //Update: Still I use lightweight `Set`s and convert them to list later.

    int len1 = list1.length();
    int len2 = list2.length();

    //Why we use Sets instead of Lists:
    //RemoveSet1 must be set to handle duplicate elements in list2, e.g L1={A}, L2={A,A}.
    //RemoveSet2 must be set to handle duplicate elements in list1, e.g L1={A,A}, L2={A}.
    QSet<int> removeSet1, removeSet2;

    for (int i1 = 0; i1 < len1; i1++)
    {
        for (int i2 = 0; i2 < len2; i2++)
        {
            if (list2[i2].compare(list1[i1], Qt::CaseInsensitive) == 0)
            {
                //These are faster than QList check for containing and inserting used up to
                //  revision BookmarkManager-201404102.
                removeSet1.insert(i1);
                removeSet2.insert(i2);
            }
        }
    }

    //Could also sort in reverse order with qSort and use an i=0..size loop, but maybe this size..0
    //  loop is faster.
    QList<int> removeList = removeSet1.toList();
    qSort(removeList.begin(), removeList.end());
    for (int r = removeList.size(); r >= 0; r--)
        list1.removeAt(removeList[r]);

    removeList = removeSet2.toList();
    qSort(removeList.begin(), removeList.end());
    for (int r = removeList.size(); r >= 0; r--)
        list2.removeAt(removeList[r]);
}

QString Util::NonExistentRandomFileNameInDirectory(const QString& dirPath, int length,
                                                   const QString& prefix, const QString& extension)
{
    QDir dir(dirPath);

    QString randomFileName;
    do
    {
        randomFileName = prefix;
        randomFileName += RandomHash(length);
        randomFileName += extension;
    } while (dir.exists(randomFileName));

    return randomFileName;
}

QString Util::UserReadableFileSize(long long size)
{
    if (size < 1024L)
        return QString::number(size) + " Bytes";
    else if (size < 1048576L)
        return QString::number(size / 1024.0L, 'f', 3) + " KiB";
    else if (size < 1073741824L)
        return QString::number(size / 1048576.0L, 'f', 3) + " MiB";
    else if (size < 1099511627776L)
        return QString::number(size / 1073741824.0L, 'f', 3) + " GiB";
    else //if (size < 1024L * 1024 * 1024 * 1024 * 1024)
        return QString::number(size / 1099511627776.0L, 'f', 3) + " TiB";
}

QByteArray Util::GetMD5HashForFile(const QString& filePathName)
{
    const int FILE_READ_CHUNK_SIZE = 65536;
    char filebuff[FILE_READ_CHUNK_SIZE];

    int bytesRead;
    QFile inFile(filePathName);

    if (inFile.open(QIODevice::ReadOnly))
    {
        QCryptographicHash hasher(QCryptographicHash::Md5);
        while (!inFile.atEnd())
        {
            bytesRead = inFile.read(filebuff, FILE_READ_CHUNK_SIZE);
            hasher.addData(filebuff, bytesRead);
        }
        inFile.close();
        return hasher.result();
    }
    else
    {
        return QByteArray(16, '\0');
    }
}

bool Util::IsValidFileName(const QString& fileName)
{
    //http://stackoverflow.com/questions/3038351/check-whether-a-string-is-a-valid-filename-with-qt
    //http://www.boost.org/doc/libs/1_43_0/libs/filesystem/doc/portability_guide.htm
    //http://stackoverflow.com/questions/62771/how-check-if-given-string-is-legal-allowed-file-name-under-windows
    //  (--> http://stackoverflow.com/a/62888/656366)

    int len = fileName.length();

    if (fileName.isEmpty())
        return false;

#if defined(Q_OS_WIN32)
    QList<QChar> invalidChars;
    invalidChars.append('<');
    invalidChars.append('>');
    invalidChars.append(':');
    invalidChars.append('"');
    invalidChars.append('/');
    invalidChars.append('\\');
    invalidChars.append('?');
    invalidChars.append('*');

    for (int i = 0; i < len; i++)
        if (fileName[i] <= 31 || invalidChars.contains(fileName[i]))
            return false;

    //We are being conservative here, we check trailing periods and spaces.
    QChar lastChar = fileName[len - 1];
    if (lastChar == ' ' || lastChar == '.')
        return false;

    //CLOCK$ is accepted nowadays, but we are being conservative.
    //Also, Windows 7 explorer throws errors with COM0 and LPT0, so we include them too.
    QStringList invalidNames;
    invalidNames << "CON" << "PRN" << "AUX" << "NUL" << "CLOCK$"
        <<"COM0" <<"COM1" <<"COM2" <<"COM3" <<"COM4" <<"COM5" <<"COM6" <<"COM7" <<"COM8" <<"COM9"
        <<"LPT0" <<"LPT1" <<"LPT2" <<"LPT3" <<"LPT4" <<"LPT5" <<"LPT6" <<"LPT7" <<"LPT8" <<"LPT9";

    if (invalidNames.contains(fileName, Qt::CaseInsensitive) ||
        invalidNames.contains(QFileInfo(fileName).baseName(), Qt::CaseInsensitive))
        return false;

#elif defined(Q_OS_LINUX) || defined(Q_OS_UNIX) || defined(Q_OS_MAC)
    //So we are extremely free here and NOT conservative, unlike on windows.
    if (fileName.contains('/'))
        return false;
#endif

    //We also don't let creating file names which are all dots.
    if (fileName.count('.') == len)
        return false;

    return true;
}

void Util::SeedRandomWithTime()
{
    srand(time(NULL));
}

int Util::Random()
{
    return rand();
}
