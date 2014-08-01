#pragma once
#include <QString>
#include <QStringList>

class FileViewHandler
{
public:
    /// Only file extensions will be verified.
    static int ChooseADefaultFileBasedOnExtension(const QStringList& filesList);

    /// Only file extension will be verified.
    static bool HasPreviewHandler(const QString& fileName);

    static void Preview(const QString& filePathName);
    static void Open(const QString& filePathName);
};
