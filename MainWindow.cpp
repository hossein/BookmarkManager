#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "BookmarkEditDialog.h"

#include <QDebug>
#include <QDir>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMessageBox>
#include <QtGui/QResizeEvent>
#include <QtGui/QToolButton>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), dbm(this, &conf)
{
    ui->setupUi(this);

    // Set size and position
    int dWidth = QApplication::desktop()->availableGeometry().width();
    int dHeight = QApplication::desktop()->availableGeometry().height();
    resize(dWidth * 75/100, dHeight * 75/100);

    QRect geom = geometry();
    geom.moveCenter(QApplication::desktop()->availableGeometry().center());
    this->setGeometry(geom);

    // Set additional UI sizes
    QList<int> sizes;
    sizes << this->width() / 3 << this->width() * 2 / 3;
    ui->splitter->setSizes(sizes);

    // Add additional UI controls
    QMenu* importMenu = new QMenu("Import");
    importMenu->addAction(ui->action_importFirefoxBookmarks);

    QToolButton* btn = new QToolButton();
    btn->setText("Import/Export");
    btn->setMenu(importMenu);
    btn->setPopupMode(QToolButton::InstantPopup);
    //btn->setArrowType(Qt::LeftArrow);
    btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    ui->mainToolBar->addWidget(btn);

    // Load the application and logic
    LoadDatabaseAndUI();

    // Additional sub-parts initialization
    dbm.files.InitializeFilesDirectory();

    qApp->postEvent(this, new QResizeEvent(this->size(), this->size()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnNew_clicked()
{
    NewBookmark();
}

void MainWindow::on_btnEdit_clicked()
{
    EditSelectedBookmark();
}

void MainWindow::on_btnDelete_clicked()
{
    DeleteSelectedBookmark();
}

void MainWindow::on_tvBookmarks_activated(const QModelIndex &index)
{
    Q_UNUSED(index);
    EditSelectedBookmark();
}

void MainWindow::tvBookmarksCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);
    register bool valid = current.isValid();
    ui->btnEdit->setEnabled(valid);
    ui->btnDelete->setEnabled(valid);
}

void MainWindow::LoadDatabaseAndUI()
{
    bool success;

    QString databaseFilePath = QDir::currentPath() + "/" + conf.nominalDatabasetFileName;
    success = dbm.BackupOpenOrCreate(databaseFilePath);
    if (!success)
        return;

    dbm.PopulateModels();
    RefreshTVBookmarksModelView();
}

void MainWindow::RefreshTVBookmarksModelView()
{
    //TODO: ^ This can have a `save selection` arg.
    //TODO: Needed at all? I mean, doesn't the model automatically update the view?

    ui->tvBookmarks->setModel(&dbm.bms.model);

    QHeaderView* hh = ui->tvBookmarks->horizontalHeader();

    BookmarkManager::BookmarkIndexes const& bidx = dbm.bms.bidx;
    if (hh->count() > 0) //This can happen on database errors.
    {
        hh->hideSection(bidx.BID);
        hh->hideSection(bidx.Desc);
        hh->hideSection(bidx.DefBFID);

        hh->setResizeMode(bidx.Name, QHeaderView::Stretch);
        hh->resizeSection(bidx.URL, 200);
        //TODO: How to show tags? hh->resizeSection(dbm.bidx.Tags, 100);
        hh->resizeSection(bidx.Rating, 50);
    }

    QHeaderView* vh = ui->tvBookmarks->verticalHeader();
    vh->setResizeMode(QHeaderView::ResizeToContents); //Disable changing row height.

    //TODO: Connect everytime?
    connect(ui->tvBookmarks->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(tvBookmarksCurrentRowChanged(QModelIndex,QModelIndex)));
}

void MainWindow::NewBookmark()
{
    BookmarkEditDialog* bmEditDialog = new BookmarkEditDialog(&dbm, -1, this);

    int result = bmEditDialog->exec();
    if (result != QDialog::Accepted)
        return;

    LoadDatabaseAndUI();
}

int MainWindow::GetSelectedBookmarkID()
{
    int selRow = ui->tvBookmarks->currentIndex().row();
    int selectedBId =
            dbm.bms.model.data(dbm.bms.model.index(selRow, dbm.bms.bidx.BID)).toLongLong();
    return selectedBId;
}

void MainWindow::ViewSelectedBookmark()
{

}

void MainWindow::EditSelectedBookmark()
{
    BookmarkEditDialog* bmEditDialog = new BookmarkEditDialog(&dbm, GetSelectedBookmarkID(), this);
    //TODO: Use `canshowthedialog` first!
    int result = bmEditDialog->exec();

    if (result != QDialog::Accepted)
        return;

    LoadDatabaseAndUI(); //TODO: Save selction
}

void MainWindow::DeleteSelectedBookmark()
{
    int selRow = ui->tvBookmarks->currentIndex().row();
    QString selectedBookmarkName =
            dbm.bms.model.data(dbm.bms.model.index(selRow, dbm.bms.bidx.Name)).toString();

    if (QMessageBox::Yes !=
        QMessageBox::question(this, "Delete Bookmark",
                              "Are you sure you want to delete the selected bookmark \""
                              + selectedBookmarkName + "\"?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
        return;

    int success = dbm.bms.DeleteBookmark(GetSelectedBookmarkID());
    if (!success)
        return;

    LoadDatabaseAndUI(); //TODO: Save scroll position
}
