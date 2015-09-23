#pragma once
#include <QString>
#include <QPixmap>

/// Note: This class won't compile on non-unicode windows; although some parts use the generic WinAPI
///       defines or 'T' versions of things instead of explicitly using the 'W' versions.

class WinFunctions
{
public:
    static bool MoveFileToRecycleBin(const QString& filePathName);

    ///Gets the display name from the first string it finds, it doesn't search for e.g English name.
    static QString GetProgramDisplayName(const QString& exePathName);
    static QPixmap GetProgramSmallIcon(const QString& exePathName);
    static QPixmap GetProgramLargeIcon(const QString& exePathName);

private:
    static QPixmap GetPixmapFromWindowsHIcon(HICON icon);
};
