#include "FileViewHandler.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

int FileViewHandler::ChooseADefaultFileBasedOnExtension(const QStringList& filesList)
{
    if (filesList.count() == 1)
        return 0;
    else if (filesList.count() == 0)
        return -1;

    QList<QStringList> extensionPriority;
    extensionPriority.append(QString("mht|mhtml|htm|html|maff"          ).split('|'));
    extensionPriority.append(QString("doc|docx|ppt|pptx|xls|xlsx|rtf"   ).split('|'));
    extensionPriority.append(QString("txt"                              ).split('|'));
    extensionPriority.append(QString("c|cpp|h|cs|bas|vb|java|py|pl"     ).split('|'));
    extensionPriority.append(QString("zip|rar|gz|bz2|7z|xz"             ).split('|'));

    QStringList filesExtList;
    foreach (const QString& fileName, filesList)
        filesExtList.append(QFileInfo(fileName).suffix().toLower());

    for (int p = 0; p < extensionPriority.size(); p++)
        for (int i = 0; i < filesExtList.size(); i++)
            if (extensionPriority[p].contains(filesExtList[i]))
                return i;

    //If the above prioritized file choosing doesn't work, simply choose the first item.
    return 0;
}

bool FileViewHandler::HasPreviewHandler(const QString& fileName)
{
    QStringList filesWithPreviewHandlers =
            QString("mht|mhtml|htm|html|maff|txt|bmp|gif|png|jpg|jpeg").split('|');
    return filesWithPreviewHandlers.contains(QFileInfo(fileName).suffix().toLower());
}
