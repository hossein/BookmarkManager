#include "Util.h"

#include <cstdlib> //rand, srand
#include <ctime> //time

#include <QCryptographicHash>
#include <QFile>

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
