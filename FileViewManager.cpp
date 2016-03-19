#include "FileViewManager.h"

#include "FileManager.h"

#include "PreviewHandlers/FilePreviewHandler.h"
#include "FilePreviewerWidget.h"
#include "OpenWithDialog.h"
#include "Util.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>

#include <QMenu>
#include <QHBoxLayout>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

#include <QUrl>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QProcess>

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

bool FileViewManager::HasPreviewHandler(const QString& fileName)
{
    return (GetPreviewHandler(fileName) != NULL);
}

FilePreviewHandler* FileViewManager::GetPreviewHandler(const QString& fileName)
{
    QString lowerSuffix = QFileInfo(fileName).suffix().toLower();
    if (m_extensionsPreviewHandlers.contains(lowerSuffix))
        return m_extensionsPreviewHandlers[lowerSuffix];
    else
        return NULL;
}

int FileViewManager::ChooseADefaultFileBasedOnExtension(const QStringList& filesList)
{
    if (filesList.count() == 1)
        return 0; //[KeepDefaultFile-1].Generalization
    else if (filesList.count() == 0)
        return -1; //[KeepDefaultFile-1].Generalization

    //TODO [handle]: Sort by the FilePreviewHandler::FileCategory categories.
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

void FileViewManager::PopulateOpenWithMenu(const QString& fileName, QMenu* parentMenu,
                                           const QObject* receiver, const char* member)
{
    long long preferredSAID = GetPreferredOpenApplication(fileName);

    //System default item
    QAction* ow_sysdefault = parentMenu->addAction(
                QIcon(":/res/exec16.png"), "Default System Application", receiver, member);
    ow_sysdefault->setData(OWS_OpenWithSystemDefault);
    if (preferredSAID == -1)
        parentMenu->setDefaultAction(ow_sysdefault);

    //Associated programs, sort alphabetically
    QMap<QString, long long> sortMap;
    foreach (long long associatedSAID, GetAssociatedOpenApplications(fileName))
        sortMap.insert(systemApps[associatedSAID].Name.toLower(), associatedSAID);

    foreach (long long associatedSAID, sortMap.values())
    {
        SystemAppData& sa = systemApps[associatedSAID];
        QAction* act = parentMenu->addAction(QIcon(sa.SmallIcon), sa.Name, receiver, member);
        act->setData(associatedSAID);
        if (preferredSAID == associatedSAID)
            parentMenu->setDefaultAction(act);
    }

    //Separator :)
    parentMenu->addSeparator();

    //Open With dialog request item
    QAction* ow_openwithreq = parentMenu->addAction(
                "Choose Default Program...", receiver, member);
    ow_openwithreq->setData(OWS_OpenWithDialogRequest);
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

void FileViewManager::PreviewStandalone(const QString& filePathName, QWidget* dialogParent)
{
    QDialog* fpdialog = new QDialog(dialogParent);
    fpdialog->setWindowTitle(QString("Preview '%1'").arg(QFileInfo(filePathName).fileName()));
    fpdialog->setContentsMargins(0, 0, 0, 0);
    fpdialog->resize(640, 480);

    //Maximize button would be more useful than a Help button on this dialog, e.g to view big pictures.
    Qt::WindowFlags flags = fpdialog->windowFlags();
    flags &= ~Qt::WindowContextHelpButtonHint;
    flags |= Qt::WindowMaximizeButtonHint;
    fpdialog->setWindowFlags(flags);

    QHBoxLayout* hlay = new QHBoxLayout(fpdialog);
    hlay->setContentsMargins(0, 0, 0, 0);
    fpdialog->setLayout(hlay);

    FilePreviewerWidget* fpw = new FilePreviewerWidget(fpdialog);
    hlay->addWidget(fpw, 1);

    Preview(filePathName, fpw);
    fpdialog->exec();
}

void FileViewManager::OpenReadOnly(const QString& filePathName, FileManager* files)
{
    GenericOpenFile(filePathName, GetPreferredOpenApplication(filePathName), true, files);
}

void FileViewManager::OpenEditable(const QString& filePathName, FileManager* files)
{
    Q_UNUSED(files)
    GenericOpenFile(filePathName, GetPreferredOpenApplication(filePathName), false, NULL);
}

void FileViewManager::OpenWith(const QString& filePathName, bool allowNonSandbox,
                               DatabaseManager* dbm, QWidget* dialogParent)
{
    //Note that we do NOT use the dbm's dialogParent; as we need to make the parent of the
    //  OpenWithDialog the previous dialog which was open, NOT the MainWindow.
    OpenWithDialog::OutParams outParams;
    OpenWithDialog openWithDlg(dbm, filePathName, allowNonSandbox, &outParams, dialogParent);

    if (!openWithDlg.canShow())
        return; //In case of errors a message is already shown.

    int result = openWithDlg.exec();
    if (result != QDialog::Accepted)
        return;

    GenericOpenFile(filePathName, outParams.selectedSAID, outParams.openSandboxed, &dbm->files);
}

void FileViewManager::SaveAs(const QString& filePathName, const QString& originalFileName,
                             DatabaseManager* dbm, QWidget* dialogParent)
{
    QFileInfo fi(filePathName);

    const QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString lastSaveAsDir = dbm->sets.GetSetting("LastSaveAsDir", documentsDir);

    QString targetFilePath = lastSaveAsDir + "/" + originalFileName;
    QString saveAsFilePath = QFileDialog::getSaveFileName(dialogParent, "Save File As",
                                                          targetFilePath, "All Files (*)");
    if (saveAsFilePath.isEmpty())
        return;

    QFileInfo saveAsInfo(saveAsFilePath);
    dbm->sets.SetSetting("LastSaveAsDir", saveAsInfo.absolutePath());

    //Remove file if it exists (user has already pressed 'Yes' on replace dialog)
    if (QFile::exists(saveAsInfo.filePath()))
    {
        if (!QFile::remove(saveAsInfo.filePath()))
        {
            QString errorText = QString("Could not replace file!\nFile: %1")
                                .arg(saveAsInfo.filePath());
            QMessageBox::critical(dialogParent, "Error", errorText);
            return;
        }
    }

    //Copy it
    if (!QFile::copy(fi.absoluteFilePath(), saveAsFilePath))
    {
        QString errorText = QString("Could not copy file!\nSource file: %1\nDestination file: %2")
                            .arg(fi.absoluteFilePath(), saveAsFilePath);
        QMessageBox::critical(dialogParent, "Error", errorText);
    }
}

void FileViewManager::ShowProperties(const QString& filePathName)
{
    //NOTE [handle]: This needs to show REAL file name and attaching date also.
}

void FileViewManager::GenericOpenFile(const QString& filePathName, long long programSAID,
                                      bool sandboxed, FileManager* files)
{
    QString filePathNameToOpen = filePathName;

    if (sandboxed)
    {
        if (!files->CopyFileToSandBoxAndGetAddress(filePathName, filePathNameToOpen))
            return; //Error on copying, etc
    }
    else
    {
        if (QMessageBox::Yes !=
            QMessageBox::question(NULL, "Open Real File", "This will modify the contents of the "
                                  "original file in the file archive. Continue?",
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
            return;
    }

    DirectOpenFile(filePathNameToOpen, programSAID);
}

void FileViewManager::DirectOpenFile(const QString& filePathName, long long programSAID)
{
    bool success;
    if (programSAID == -1)
    {
        //This QUrl override is tolerant and changes URLs to `file://` type.
        success = QDesktopServices::openUrl(QUrl(filePathName));
    }
    else
    {
        success = QProcess::startDetached(systemApps[programSAID].Path, QStringList() << filePathName);
    }

    if (!success)
        QMessageBox::critical(NULL, "Error opening file",
                              QString("Error opening file.\nFile: %1\nProgram: %2")
                              .arg(filePathName,
                                   programSAID == -1
                                   ? "Default System Application"
                                   : systemApps[programSAID].Path));
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

bool FileViewManager::DeleteSystemAppAndAssociationsAndPreference(long long SAID)
{
    //We use SQLite foreign keys and CASCADING deletes to delete the data from SystemApp and all
    //  tables which reference the SAID (i.e ExtAssoc and ExtOpenWith) without need to transactions.

    QString deleteSystemAppError = "Could not remove program from database.";

    QSqlQuery query(db);
    query.prepare("DELETE FROM SystemApp WHERE SAID = ?");
    query.addBindValue(SAID);

    //We are using foreign keys that CASCADE on delete, so executing that query also removes the
    //  entries from ExtAssoc and ExtOpenWith tables. Removing from ExtAssoc table simply removes
    //  the program from the associated, 'bold' programs for that extension. Removing from
    //  ExtOpenWith table causes the preferred application to become system default (-1).
    if (!query.exec())
        return Error(deleteSystemAppError, query.lastError());

    //We are [RESPONSIBLE] for updating the internal tables, all three of them.
    systemApps.remove(SAID);

    for (auto it = associatedOpenPrograms.begin(); it != associatedOpenPrograms.end(); ++it)
        it.value().removeAll(SAID);

    for (auto it = preferredOpenProgram.begin(); it != preferredOpenProgram.end(); )
        if (it.value().SAID == SAID)
            it = preferredOpenProgram.erase(it);
        else
            ++it;

    return true;
}

QList<long long> FileViewManager::GetAssociatedOpenApplications(const QString& fileName)
{
    QString lowerSuffix = QFileInfo(fileName).suffix().toLower();
    if (associatedOpenPrograms.contains(lowerSuffix))
        return associatedOpenPrograms[lowerSuffix];
    else
        return QList<long long>();
}

bool FileViewManager::AssociateApplicationWithExtension(const QString& fileName,
                                                        long long associatedSAID)
{
    QString lowerSuffix = QFileInfo(fileName).suffix().toLower();

    if (associatedSAID == -1)
        return true;

    if (associatedOpenPrograms.contains(lowerSuffix)
        && associatedOpenPrograms[lowerSuffix].contains(associatedSAID))
        return true; //Already associated.

    QString setAssociatedSAIDError =
            "Could not alter associated programs information for file extension in the database.";

    QSqlQuery query(db);
    query.prepare("INSERT INTO ExtAssoc (LExtension, SAID) VALUES (?, ?)");

    query.addBindValue(lowerSuffix);
    query.addBindValue(associatedSAID);

    if (!query.exec())
        return Error(setAssociatedSAIDError, query.lastError());

    //We are [RESPONSIBLE] for updating the internal tables.
    //This always works since QHash returns a default-constructed QList when lowerSuffix doesn't exist.
    associatedOpenPrograms[lowerSuffix].append(associatedSAID);

    return true;
}

bool FileViewManager::UnAssociateApplicationWithExtension(const QString& fileName,
                                                          long long associatedSAID)
{
    QString lowerSuffix = QFileInfo(fileName).suffix().toLower();

    QString unAssociateError =
            "Could not remove associated programs information for file extension from the database.";

    QSqlQuery query(db);
    query.prepare("DELETE FROM ExtAssoc WHERE LExtension = ? AND SAID = ?");

    query.addBindValue(lowerSuffix);
    query.addBindValue(associatedSAID);

    if (!query.exec())
        return Error(unAssociateError, query.lastError());

    //We are [RESPONSIBLE] for updating the internal tables.
    associatedOpenPrograms[lowerSuffix].removeAll(associatedSAID);

    return true;
}

QList<QString> FileViewManager::GetExtensionsWithWhichApplicationIsAssociated(long long SAID)
{
    QList<QString> extensionList;
    for (auto it = associatedOpenPrograms.constBegin(); it != associatedOpenPrograms.constEnd(); ++it)
        if (it.value().contains(SAID))
            extensionList.append(it.key());
    return extensionList;
}

long long FileViewManager::GetPreferredOpenApplication(const QString& fileName)
{
    QString lowerSuffix = QFileInfo(fileName).suffix().toLower();
    if (preferredOpenProgram.contains(lowerSuffix))
        return preferredOpenProgram[lowerSuffix].SAID;
    else
        return -1;
}

bool FileViewManager::SetPreferredOpenApplication(const QString& fileName, long long preferredSAID)
{
    QString lowerSuffix = QFileInfo(fileName).suffix().toLower();
    long long EOWID = -1;
    if (preferredOpenProgram.contains(lowerSuffix))
        EOWID = preferredOpenProgram[lowerSuffix].EOWID;

    QString setPreferredSAIDError =
            "Could not alter preferred program information for file in the database.";

    QString querystr;
    if (EOWID == -1)
        querystr = "INSERT INTO ExtOpenWith (LExtension, SAID) VALUES (?, ?)";
    else
        querystr = "UPDATE ExtOpenWith SET LExtension = ?, SAID = ? WHERE EOWID = ?";

    QSqlQuery query(db);
    query.prepare(querystr);

    query.addBindValue(lowerSuffix);
    query.addBindValue(preferredSAID);

    if (EOWID != -1)
        query.addBindValue(EOWID); //Edit

    if (!query.exec())
        return Error(setPreferredSAIDError, query.lastError());

    if (EOWID == -1)
        EOWID = query.lastInsertId().toLongLong(); //Get real EOWID value after insert.

    //We are [RESPONSIBLE] for updating the internal tables.
    preferredOpenProgram[lowerSuffix] = ExtOpenWithData(EOWID, preferredSAID);

    return true;
}

QList<QString> FileViewManager::GetExtensionsForWhichApplicationIsPreffered(long long SAID)
{
    QList<QString> extensionList;
    for (auto it = preferredOpenProgram.constBegin(); it != preferredOpenProgram.constEnd(); ++it)
        if (it.value().SAID == SAID)
            extensionList.append(it.key());
    return extensionList;
}

void FileViewManager::CreateTables()
{
    //IMPORTANT:
    //Rationale for having separate OpenWith tables with the system:
    //  Especially on Windows, it is easy to get the Open With list Explorer uses, however
    //  that list is usually cluttered and also for some file types, e.g EXE doesn't allow
    //  customization without registry hacking, and also deleting entries from it is not easy.

    //Preferred Applications vs Associated Applications:
    //  Each file extension (aka type or suffix) has one Preferred application and multiple
    //  Associated Applications. Associated applications are a list of programs which the user
    //  has ever used to open the file extension with. Preferred application is the application
    //  that user prefers the file to be opened with that application upon Double-Click.
    //  'System Default' (usually -1) can be a Preferred application BUT NEVER an associated one.

    //Extensions are meant to be LOWERCASE WITHOUT THE FIRST DOT character.
    //There is also a special extension called '' (empty string!) which appears in OpenWith list
    //  of all files after the extension's specialized openers, before the others.
    //TODO [handle]: ^ No there isn't a '' extension! We should make special care in Open with dialog preference,
    //  etc to make sure we exempt '' from other extensions even!

    QSqlQuery query(db);

    query.exec("CREATE TABLE SystemApp"
               "( SAID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, Path TEXT, "
               "  SmallIcon BLOB, LargeIcon BLOB )");

    //Note: After supporting foreign keys, we get error if we want to set a Preferred app to -1
    //  in ExtOpenWith table, because there is no application with SAID=-1 in SystemApp.
    //  We simply insert dummy -1 in SystemApp table such that this constraint is not violated.
    //  The more correct solution is NOT to store -1's in the DB at all.
    query.exec("INSERT INTO SystemApp(SAID) VALUES (-1)");

    //Extension Association Table: programs that were recently used to open that extension.
    query.exec("CREATE TABLE ExtAssoc"
               "( EAID INTEGER PRIMARY KEY AUTOINCREMENT, LExtension TEXT, SAID INTEGER, "
               "  FOREIGN KEY(SAID) REFERENCES SystemApp(SAID) ON DELETE CASCADE "
               ")");

    //Extension Open With Table: User prefers to open an extension with System Default or an app.
    query.exec("CREATE TABLE ExtOpenWith"
               "( EOWID INTEGER PRIMARY KEY AUTOINCREMENT, LExtension TEXT, SAID INTEGER, "
               "  FOREIGN KEY(SAID) REFERENCES SystemApp(SAID) ON DELETE CASCADE "
               ")");
}

void FileViewManager::PopulateModelsAndInternalTables()
{
    //FileViewManager does not have any models.

    //Instead we populate the internal fast-access lists.
    //Note: We do NOT do this at `FileViewManager::PopulateModelsAndInternalTables()` as it is
    //      called multiple times and upon Bookmark/Tag/File/etc add or edit and is expensive.
    //So we do it at this function ONLY ONCE and then the database functions directly modify
    //      `this->systemApps` instead of re-retrieval from DB each time.

    /// System Applications Table /////////////////////////////////////////////////////////////////
    QString retrieveError = "Could not get programs information from the database.";
    QSqlQuery query(db);
    query.prepare("SELECT * FROM SystemApp WHERE SAID >= 0");

    if (!query.exec())
    {
        Error(retrieveError, query.lastError());
        return;
    }

    systemApps.clear();
    while (query.next())
    {
        SystemAppData sa;
        const QSqlRecord record = query.record();
        sa.SAID      = record.value("SAID").toLongLong();
        sa.Name      = record.value("Name").toString();
        sa.Path      = record.value("Path").toString();
        sa.SmallIcon = Util::DeSerializeQPixmap(record.value("SmallIcon").toByteArray());
        sa.LargeIcon = Util::DeSerializeQPixmap(record.value("LargeIcon").toByteArray());

        systemApps[sa.SAID] = sa;
    }

    /// Extension Associations Table //////////////////////////////////////////////////////////////
    retrieveError = "Could not get extention program associations information from database.";
    query.prepare("SELECT * FROM ExtAssoc");

    if (!query.exec())
    {
        Error(retrieveError, query.lastError());
        return;
    }

    associatedOpenPrograms.clear();
    while (query.next())
    {
        const QSqlRecord record = query.record();
        QString lowerSuffix = record.value("LExtension").toString();
        long long associatedSAID = record.value("SAID").toLongLong();
        //This always works since QHash returns a default-constructed QList when value at lowerSuffix
        //  doesn't exist.
        associatedOpenPrograms[lowerSuffix].append(associatedSAID);
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
        const QSqlRecord record = query.record();
        long long EOWID = record.value("EOWID").toLongLong();
        QString lowerSuffix = record.value("LExtension").toString();
        long long preferredSAID = record.value("SAID").toLongLong();
        preferredOpenProgram[lowerSuffix] = ExtOpenWithData(EOWID, preferredSAID);
    }
}
