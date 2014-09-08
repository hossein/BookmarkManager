#include "FilePreviewHandler.h"

#include "FileViewManager.h"

#include "PreviewHandlers/LocalHTMLPreviewHandler.h"

void FilePreviewHandler::InstantiateAllKnownFilePreviewHandlersInFileViewManager(FileViewManager* fview)
{
    fview->AddPreviewHandler(new LocalHTMLPreviewHandler());
}
