#include "FileViewManager.h"

#include "FileManager.h"

#include "PreviewHandlers/FilePreviewHandler.h"
#include "FilePreviewerWidget.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

#include <QUrl>
#include <QDesktopServices>

FileViewManager::FileViewManager(QWidget* dialogParent, Config* conf)
    : ISubManager(dialogParent, conf)
{
    //Add all file preview handlers to this.
    FilePreviewHandler::InstantiateAllKnownFilePreviewHandlersInFileViewManager(this);
}

FileViewManager::~FileViewManager()
{
    foreach (FilePreviewHandler* fph, m_ownedPreviewHandlers)
        delete fph;
}

void FileViewManager::AddPreviewHandler(FilePreviewHandler* fph)
{
    QStringList fphExtensions = fph->GetSupportedExtensions();
    foreach (const QString& ext, fphExtensions)
        m_extensionsPreviewHandlers[ext] = fph;
}

int FileViewManager::ChooseADefaultFileBasedOnExtension(const QStringList& filesList)
{
    if (filesList.count() == 1)
        return 0;
    else if (filesList.count() == 0)
        return -1;

    //TODO: Sort by the FilePreviewHandler::FileCategory categories.
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

bool FileViewManager::HasPreviewHandler(const QString& fileName)
{
    return (GetPreviewHandler(fileName) != NULL);
}

void FileViewManager::Preview(const QString& filePathName, FilePreviewerWidget* fpw)
{
    //We just do an additional check, although user should be careful not to call this function
    //  for files without preview handlers.
    FilePreviewHandler* fph = GetPreviewHandler(filePathName);
    if (fph == NULL)
        return;

    fpw->PreviewFileUsingPreviewHandler(filePathName, fph);
}

void FileViewManager::OpenReadOnly(const QString& filePathName, FileManager* files)
{
    QString sandBoxFilePathName = files->CopyFileToSandBoxAndGetAddress(filePathName);

    if (sandBoxFilePathName.isEmpty()) //Error on copying, etc
        return;

    QDesktopServices::openUrl(QUrl(sandBoxFilePathName));
}

void FileViewManager::OpenEditable(const QString& filePathName, FileManager* files)
{
    Q_UNUSED(files)
    QDesktopServices::openUrl(QUrl(filePathName));
}

void FileViewManager::OpenWith(const QString& filePathName, FileManager* files)
{

}

void FileViewManager::ShowProperties(const QString& filePathName)
{
    //NOTE: This needs to show REAL file name and attaching date also.
}

FilePreviewHandler* FileViewManager::GetPreviewHandler(const QString& fileName)
{
    QString extension = QFileInfo(fileName).suffix().toLower();
    if (m_extensionsPreviewHandlers.contains(extension))
        return m_extensionsPreviewHandlers[extension];
    else
        return NULL;
}

void FileViewManager::CreateTables()
{
    //IMPORTANT:
    //Rationale for having separate OpenWith tables with the system:
    //  Especially on Windows, it is easy to get the Open With list Explorer uses, however
    //  that list is usually cluttered and also for some file types, e.g EXE doesn't allow
    //  customization without registry hacking, and also deleting entries from it is not easy.

    //Extensions are meant to be LOWERCASE WITHOUT THE FIRST DOT character.
    //There is also a special extension called '' (empty string!) which appears in OpenWith list
    //  of all files after the extension's specialized openers, before the others.

    QSqlQuery query(db);

    query.exec("CREATE TABLE SystemApp"
               "( SAID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, PATH TEXT )");

    //Extension Association Table
    query.exec("CREATE TABLE ExtAssoc"
               "( EAID INTEGER PRIMARY KEY AUTOINCREMENT, Extension TEXT, SAID INTEGER )");
}

void FileViewManager::PopulateModels()
{
    //FileViewManager does not have any models.
}
