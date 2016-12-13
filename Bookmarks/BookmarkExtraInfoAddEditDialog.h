#pragma once
#include <QDialog>
#include "BookmarkManager.h"

namespace Ui { class BookmarkExtraInfoAddEditDialog; }

class BookmarkExtraInfoAddEditDialog : public QDialog
{
    Q_OBJECT

public:
    struct EditedProperty
    {
        BookmarkManager::BookmarkExtraInfoData::DataType Type;
        QString Name;
        QString Value;
    };

    explicit BookmarkExtraInfoAddEditDialog(EditedProperty* editedProperty, QWidget *parent = 0);
    ~BookmarkExtraInfoAddEditDialog();

public slots:
    void accept();

private slots:
    void on_cboType_currentIndexChanged(int index);

private:
    Ui::BookmarkExtraInfoAddEditDialog *ui;
    EditedProperty* m_editedProperty;
};
