#include "WinFunctions.h"

#include <string>
#include <Windows.h>

bool MoveFileToRecycleBin(const QString& filePathName)
{
    //TODO: Check with unicode file names. and if delete returns 0 on a non-existent file or not!
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
    fileOp.fFlags = FOF_ALLOWUNDO /* | FOF_NOCONFIRMATION */ | FOF_NOERRORUI /* | FOF_SILENT| FOF_WANTNUKEWARNING */;

    int result = SHFileOperationW(&fileOp);

    delete wsFilePathName;

    return (result == 0);
}
