#include "BookmarkExtraInfoAddEditDialog.h"
#include "ui_BookmarkExtraInfoAddEditDialog.h"

#include <QMessageBox>

BookmarkExtraInfoAddEditDialog::BookmarkExtraInfoAddEditDialog(EditedProperty* editedProperty, QWidget *parent) :
    QDialog(parent), ui(new Ui::BookmarkExtraInfoAddEditDialog), m_editedProperty(editedProperty)
{
    ui->setupUi(this);

    if (m_editedProperty == NULL)
        return;

    ui->leName->setText(m_editedProperty->Name);
    ui->cboType->setCurrentIndex(static_cast<int>(m_editedProperty->Type));
    ui->leValue->setText(m_editedProperty->Value);
}

BookmarkExtraInfoAddEditDialog::~BookmarkExtraInfoAddEditDialog()
{
    delete ui;
}

void BookmarkExtraInfoAddEditDialog::accept()
{
    //Validate
    //Note that we deliberately do not check if values for types Number and Boolean are correct. We allow freeform values.
    if (ui->leName->text().isEmpty())
    {
        QMessageBox::warning(this, "Error", "Entering the property name is mandatory.");
        return;
    }

    if (m_editedProperty != NULL)
    {
        m_editedProperty->Name = ui->leName->text();
        m_editedProperty->Type =
            static_cast<BookmarkManager::BookmarkExtraInfoData::DataType>(ui->cboType->currentIndex());

        if (ui->cboType->currentIndex() == 0)
            m_editedProperty->Value = QString();
        else
            m_editedProperty->Value = ui->leValue->text();
    }

    QDialog::accept();
}

void BookmarkExtraInfoAddEditDialog::on_cboType_currentIndexChanged(int index)
{
    ui->leValue->setEnabled(index > 0);
}
