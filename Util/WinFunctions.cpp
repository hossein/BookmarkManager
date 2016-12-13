#include "WinFunctions.h"

#include <QFileInfo>
#include <QtWinExtras/QtWin>

#include <Windows.h>

bool WinFunctions::MoveFileToRecycleBin(const QString& filePathName)
{
    //Unicode file names work in the following. Deleting non-existent files or deleting files whose
    //  path names is longer than MAX_PATH results in DE_INVALIDFILES result (124, 0x7C).
    //Appending "\\?\" before the file names does not solve the problem of MAX_PATH; it just causes
    //  everything to fail as SHFileOperation does not support it:
    //  http://blogs.msdn.com/b/emreknlk/archive/2010/12/03/remove-max-path-limitations.aspx
    //Actually docs suggest using the new IFileOperation in Vista+.

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
    //FOF_WANTNUKEWARNING can be useful to prevent permanently deleting e.g large files or those
    //  with long names. As per the docs it partially overrides FOF_NOCONFIRMATION, however it
    //  doesn't block our UI and user can cancel it. So won't use it.
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT /*| FOF_WANTNUKEWARNING*/;

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

    QString failSafeName = QFileInfo(exePathName).completeBaseName(); //We do NOT include the '.exe'

    //http://stackoverflow.com/questions/940707/how-do-i-programatically-get-the-version-of-a-dll-or-exe-file
    DWORD verHandle = NULL;
    DWORD verSize = GetFileVersionInfoSize((LPCWSTR)wsFilePathName, &verHandle);
    if (verSize == 0) //Error
        return failSafeName;

    LPSTR verData = new char[verSize]; //`char` should be correct, as verSize is in bytes.
    BOOL success = GetFileVersionInfo((LPCWSTR)wsFilePathName, verHandle, verSize, verData);
    if (success == 0) //Error
        return failSafeName;

    //http://msdn.microsoft.com/en-us/library/windows/desktop/ms647464%28v=vs.85%29.aspx
    struct LANGUAGECODEPAGE
    {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;
    UINT lpTranslateSize;
    success = VerQueryValue(verData, L"\\VarFileInfo\\Translation",
                            (LPVOID*)&lpTranslate, &lpTranslateSize);

    //Note: We can search user's language in the array, but we don't.
    //Now we pick JUST THE FIRST LANG! It's either English or not, whatever.

    //We assume this function always succeeds.
    WCHAR QueryWStr[100];
    wsprintf(QueryWStr, L"\\StringFileInfo\\%04x%04x\\FileDescription",
             lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);

    LPWSTR lpBuffer = NULL;
    UINT size = 0;
    success = VerQueryValue(verData, QueryWStr, (LPVOID*)&lpBuffer, &size);
    if (success == 0) //Error
        return failSafeName;

    //The `size` is size of bytes, not characters! So we have a null character at the end.
    //  See http://blogs.msdn.com/b/oldnewthing/archive/2006/12/22/1348663.aspx.
    //  Let's assume it's not just always one zero byte, or it ever is. So use a while to delete
    //  all, if any, zero bytes.
    while (lpBuffer[size - 1] == L'\0')
        size -= 1;

    //Note: QString::toWCharArray produces UTF-16, BUT QString::fromWCharArray assumes input is UCS-2!
    //      QString::fromUtf16 gets input from UTF-16.
    QString displayName = QString::fromUtf16((ushort*)lpBuffer, size); //The size is correct.

    delete[] verData;

    return displayName;
}

QPixmap WinFunctions::GetProgramSmallIcon(const QString& exePathName)
{
    int len = exePathName.length();
    wchar_t* wsFilePathName = new wchar_t[len + 1];
    exePathName.toWCharArray(wsFilePathName);
    wsFilePathName[len + 0] = '\0'; //Qt Doesn't put null-terminator there.

    //Note: I didn't call Co/OleInitialize before all callings of this function in this code
    //      as required by documentation. Is it needed? Is it called by Qt already?
    SHFILEINFOW sfi = {0};
    int ret = SHGetFileInfoW((LPCWSTR)wsFilePathName, -1, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON);
    if (ret == 0) //Error
        return QPixmap();

    QPixmap pixmap = GetPixmapFromWindowsHIcon(sfi.hIcon);
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

    QPixmap pixmap = GetPixmapFromWindowsHIcon(sfi.hIcon);
    DestroyIcon(sfi.hIcon);

    return pixmap;
}

QPixmap WinFunctions::GetPixmapFromWindowsHIcon(HICON icon)
{
    //Qt4: return QPixmap::fromWinHICON(icon);
    return QtWin::fromHICON(icon);
}
