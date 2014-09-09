#include "FilePreviewHandler.h"

#include "FileViewManager.h"

#include "PreviewHandlers/LocalHTMLPreviewHandler.h"
#include "PreviewHandlers/ImagePreviewHandler.h"

void FilePreviewHandler::InstantiateAllKnownFilePreviewHandlersInFileViewManager(FileViewManager* fview)
{
    fview->AddPreviewHandler(new LocalHTMLPreviewHandler());
    fview->AddPreviewHandler(new ImagePreviewHandler());
}
