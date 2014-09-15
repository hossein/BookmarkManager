#include "WinFunctions.h"

#include <QFileInfo>

#include <Windows.h>

bool WinFunctions::MoveFileToRecycleBin(const QString& filePathName)
{
    //TODO: Check with unicode file names. and if delete returns 0 on a non-existent file or not!
    //      Should we append \\\\?\\ or whatever? http://blogs.msdn.com/b/emreknlk/archive/2010/12/03/remove-max-path-limitations.aspx

    int len = filePathName.length();
    wchar_t* wsFilePathName = new wchar_t[len + 2];
    filePathName.toWCharArray(wsFilePathName);
    wsFilePathName[len + 0] = '\0'; //Qt Doesn't put null-terminator there.
    wsFilePathName[len + 1] = '\0'; //Must be double null terminated.

    SHFILEOPSTRUCT fileOp;
    fileOp.hwnd = NULL;
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = wsFilePathName;
    fileOp.pTo = NULL;
    //TODO: Test these flags and enable all of them...
    fileOp.fFlags = FOF_ALLOWUNDO /* | FOF_NOCONFIRMATION */ | FOF_NOERRORUI /* | FOF_SILENT| FOF_WANTNUKEWARNING */;

    int result = SHFileOperationW(&fileOp);

    delete wsFilePathName;

    return (result == 0);
}

QString WinFunctions::GetProgramDisplayName(const QString& exePathName)
{
    int len = exePathName.length();
    wchar_t* wsFilePathName = new wchar_t[len + 1];
    exePathName.toWCharArray(wsFilePathName);
    wsFilePathName[len + 0] = '\0'; //Qt Doesn't put null-terminator there.

    //Note: I didn't call Co/OleInitialize before all callings of this function in this code
    //      as required by documentation. Is it needed? Is it called by Qt already?
    SHFILEINFOW sfi = {0};
    int ret = SHGetFileInfoW((LPCWSTR)wsFilePathName, -1, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME);
    if (ret == 0) //Error
        //We do NOT include the '.exe', however the above API call does include it if user has
        //  configured Windows to always show extensions and the EXE has not set a display name
        //  in its resources.
        return QFileInfo(exePathName).completeBaseName();

    //Note: QString::toWCharArray produces UTF-16, BUT QString::fromWCharArray assumes input is UCS-2!
    //      QString::fromUtf16 gets input from UTF-16.
    QString displayName = QString::fromUtf16(sfi.szDisplayName);
    return displayName;
}

QPixmap WinFunctions::GetProgramSmallIcon(const QString& exePathName)
{
    int len = exePathName.length();
    wchar_t* wsFilePathName = new wchar_t[len + 1];
    exePathName.toWCharArray(wsFilePathName);
    wsFilePathName[len + 0] = '\0'; //Qt Doesn't put null-terminator there.

    SHFILEINFOW sfi = {0};
    int ret = SHGetFileInfoW((LPCWSTR)wsFilePathName, -1, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON);
    if (ret == 0) //Error
        return QPixmap();

    QPixmap pixmap = QPixmap::fromWinHICON(sfi.hIcon);
    DestroyIcon(sfi.hIcon);

    return pixmap;
}

QPixmap WinFunctions::GetProgramLargeIcon(const QString& exePathName)
{
    int len = exePathName.length();
    wchar_t* wsFilePathName = new wchar_t[len + 1];
    exePathName.toWCharArray(wsFilePathName);
    wsFilePathName[len + 0] = '\0'; //Qt Doesn't put null-terminator there.

    SHFILEINFOW sfi = {0};
    int ret = SHGetFileInfoW((LPCWSTR)wsFilePathName, -1, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON);
    if (ret == 0) //Error
        return QPixmap();

    QPixmap pixmap = QPixmap::fromWinHICON(sfi.hIcon);
    DestroyIcon(sfi.hIcon);

    return pixmap;
}
