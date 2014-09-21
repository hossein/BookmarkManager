#include "FileViewManager.h"

#include "FileManager.h"

#include "PreviewHandlers/FilePreviewHandler.h"
#include "FilePreviewerWidget.h"
#include "OpenWithDialog.h"
#include "Util.h"

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

void FileViewManager::OpenWith(const QString& filePathName, DatabaseManager* dbm)
{
    //TODO: Incomplete
    OpenWithDialog::OutParams outParams;
    OpenWithDialog openWithDlg(dbm, filePathName, &outParams,
                               NULL /* TODO: We don't have a parent to pass */);

    if (!openWithDlg.canShow())
        return; //In case of errors a message is already shown.

    int result = openWithDlg.exec();
    if (result != QDialog::Accepted)
        return;
}

void FileViewManager::ShowProperties(const QString& filePathName)
{
    //NOTE: This needs to show REAL file name and attaching date also.
}

FilePreviewHandler* FileViewManager::GetPreviewHandler(const QString& fileName)
{
    QString lowerSuffix = QFileInfo(fileName).suffix().toLower();
    if (m_extensionsPreviewHandlers.contains(lowerSuffix))
        return m_extensionsPreviewHandlers[lowerSuffix];
    else
        return NULL;
}

void FileViewManager::PopulateInternalTables()
{
    //Instead we populate the internal fast-access lists.
    //Note: We do NOT do this at `FileViewManager::PopulateModels()` as it is called multiple times
    //      and upon Bookmark/Tag/File/etc add or edit and is expensive.
    //So we do it at this function ONLY ONCE and then the database functions directly modify
    //      `this->systemApps` instead of re-retrieval from DB each time.

    /// System Applications Table /////////////////////////////////////////////////////////////////
    QString retrieveError = "Could not get programs information from the database.";
    QSqlQuery query(db);
    query.prepare("SELECT * FROM SystemApp");

    if (!query.exec())
    {
        Error(retrieveError, query.lastError());
        return;
    }

    systemApps.clear();
    while (query.next())
    {
        SystemAppData sa;
        QSqlRecord record = query.record();
        sa.SAID      = record.value("SAID").toLongLong();
        sa.Name      = record.value("Name").toString();
        sa.Path      = record.value("Path").toString();
        sa.SmallIcon = Util::DeSerializeQPixmap(record.value("SmallIcon").toByteArray());
        sa.LargeIcon = Util::DeSerializeQPixmap(record.value("LargeIcon").toByteArray());

        systemApps[sa.SAID] = sa;
    }

    /// Extension Open With Table /////////////////////////////////////////////////////////////////
    retrieveError = "Could not get preferred program information from database.";
    query.prepare("SELECT * FROM ExtOpenWith");

    if (!query.exec())
    {
        Error(retrieveError, query.lastError());
        return;
    }

    preferredOpenProgram.clear();
    while (query.next())
    {
        QSqlRecord record = query.record();
        QString lowerSuffix = record.value("LExtension").toString();
        long long preferredSAID = record.value("SAID").toLongLong();
        preferredOpenProgram[lowerSuffix] = preferredSAID;
    }
}

bool FileViewManager::AddOrEditSystemApp(long long& SAID, FileViewManager::SystemAppData& sadata)
{
    QString updateError = (SAID == -1
                           ? "Could not add program information to database."
                           : "Could not edit program information.");

    QString querystr;
    if (SAID == -1)
    {
        querystr =
                "INSERT INTO SystemApp (Name, Path, SmallIcon, LargeIcon) "
                "VALUES (?, ?, ?, ?)";
    }
    else
    {
        querystr =
                "UPDATE SystemApp "
                "SET Name = ?, Path = ?, SmallIcon = ?, LargeIcon = ? "
                "WHERE SAID = ?";
    }

    QSqlQuery query(db);
    query.prepare(querystr);

    query.addBindValue(sadata.Name);
    query.addBindValue(sadata.Path);
    query.addBindValue(Util::SerializeQPixmap(sadata.SmallIcon));
    query.addBindValue(Util::SerializeQPixmap(sadata.LargeIcon));

    if (SAID != -1)
        query.addBindValue(SAID); //Edit

    if (!query.exec())
        return Error(updateError, query.lastError());

    if (SAID == -1)
    {
        long long addedSAID = query.lastInsertId().toLongLong();
        SAID = addedSAID;
        sadata.SAID = addedSAID;
    }

    //We are [RESPONSIBLE] for updating the internal tables, too! Both on Add and Edit.
    systemApps[SAID] = sadata;

    return true;
}

long long FileViewManager::GetPreferredOpenApplication(const QString& fileName)
{
    QString lowerSuffix = QFileInfo(fileName).suffix().toLower();
    if (preferredOpenProgram.contains(lowerSuffix))
        return preferredOpenProgram[lowerSuffix];
    else
        return -1;
}

bool FileViewManager::SetPreferredOpenApplication(const QString& fileName, long long preferredSAID)
{
    QString lowerSuffix = QFileInfo(fileName).suffix().toLower();

    QString setPreferredSAIDError =
            "Could not alter preferred program information for file in the database.";

    //TODO: Doesn't primary key get NULL on REPLACE?
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE ExtOpenWith SET SAID = ? WHERE LExtension = ?");
    query.addBindValue(preferredSAID);
    query.addBindValue(lowerSuffix);

    if (!query.exec())
        return Error(setPreferredSAIDError, query.lastError());

    //We are [RESPONSIBLE] for updating the internal tables.
    preferredOpenProgram[lowerSuffix] = preferredSAID;

    return true;
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
               "( SAID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, Path TEXT, "
               "  SmallIcon BLOB, LargeIcon BLOB )");

    //Extension Association Table: programs that were recently used to open that extension.
    query.exec("CREATE TABLE ExtAssoc"
               "( EAID INTEGER PRIMARY KEY AUTOINCREMENT, LExtension TEXT, SAID INTEGER )");

    //Extension Open With Table: User prefers to open an extension with System Default or an app.
    query.exec("CREATE TABLE ExtOpenWith"
               "( EOWID INTEGER PRIMARY KEY AUTOINCREMENT, LExtension TEXT, SAID INTEGER )");
}

void FileViewManager::PopulateModels()
{
    //FileViewManager does not have any models.
}
