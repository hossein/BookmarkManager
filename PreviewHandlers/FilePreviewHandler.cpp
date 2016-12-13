#include "FilePreviewHandler.h"

#include "FileViewer/FileViewManager.h"

#include "PreviewHandlers/LocalHTMLPreviewHandler.h"
#include "PreviewHandlers/ImagePreviewHandler.h"
#include "PreviewHandlers/TextPreviewHandler.h"

void FilePreviewHandler::InstantiateAllKnownFilePreviewHandlersInFileViewManager(FileViewManager* fview)
{
    fview->AddPreviewHandler(new LocalHTMLPreviewHandler());
    fview->AddPreviewHandler(new ImagePreviewHandler());
    fview->AddPreviewHandler(new TextPreviewHandler());
}
