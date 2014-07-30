#include "Util.h"

#include <cstdlib> //rand, srand
#include <ctime> //time

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

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
    //TODO: Use `Set`s at a later version.

    int len1 = list1.length();
    int len2 = list2.length();
    QList<int> removeList1, removeList2;

    for (int i1 = 0; i1 < len1; i1++)
    {
        for (int i2 = 0; i2 < len2; i2++)
        {
            if (list2[i2].compare(list1[i1], Qt::CaseInsensitive) == 0)
            {
                //This condition is for handling duplicate elements in list2, e.g L1={A}, L2={A,A}.
                if (!removeList1.contains(i1))
                    removeList1.append(i1);

                //This condition is for handling duplicate elements in list1, e.g L1={A,A}, L2={A}.
                if (!removeList2.contains(i2))
                    removeList2.append(i2);
            }
        }
    }

    for (int r = removeList1.size(); r >= 0; r--)
        list1.removeAt(removeList1[r]);

    for (int r = removeList2.size(); r >= 0; r--)
        list2.removeAt(removeList2[r]);
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
    else if (size < 1024L * 1024)
        return QString::number(size / 1024.0L, 'f', 3) + " KiB";
    else if (size < 1024L * 1024 * 1024)
        return QString::number(size / 1048576.0L, 'f', 3) + " MiB";
    else if (size < 1024L * 1024 * 1024 * 1024)
        return QString::number(size / 1073741824.0L, 'f', 3) + " GiB";
    else //if (size < 1024L * 1024 * 1024 * 1024 * 1024)
        return QString::number(size / 1099511627776.0L, 'f', 3) + " TiB";
}

QByteArray Util::GetMD5HashForFile(const QString& fileName)
{
    const int FILE_READ_CHUNK_SIZE = 65536;
    char filebuff[FILE_READ_CHUNK_SIZE];

    int bytesRead;
    QFile inFile(fileName);

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

void Util::SeedRandomWithTime()
{
    srand(time(NULL));
}

int Util::Random()
{
    return rand();
}
