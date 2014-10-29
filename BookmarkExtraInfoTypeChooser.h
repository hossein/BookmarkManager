#pragma once
#include <QStyledItemDelegate>
#include <QComboBox>

class BookmarkExtraInfoTypeChooser : public QStyledItemDelegate
{
    Q_OBJECT

private:
    QStringList dataTypeNames;

public:
    BookmarkExtraInfoTypeChooser(QWidget* parent) : QStyledItemDelegate(parent)
    {
        //These must correspong to BookmarkManager::BookmarkExtraInfoData::DataType enum values.
        dataTypeNames << "Null" << "Text" << "Number" << "Boolean";
    }

    // QAbstractItemDelegate interface
    QWidget *createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        Q_UNUSED(option)
        Q_UNUSED(index)

        QComboBox* cboItems = new QComboBox(parent);
        cboItems->addItems(dataTypeNames);

        return cboItems;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        int value = index.model()->data(index, Qt::EditRole).toInt();
        QComboBox* cboItems = static_cast<QComboBox*>(editor);
        cboItems->setCurrentIndex(value);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        QComboBox* cboItems = static_cast<QComboBox*>(editor);
        int value = cboItems->currentIndex();
        model->setData(index, value, Qt::EditRole);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        Q_UNUSED(index);
        editor->setGeometry(option.rect);
    }

    // QStyledItemDelegate interface
    QString displayText(const QVariant& value, const QLocale& locale) const
    {
        Q_UNUSED(locale)

        bool okay;
        int intValue = 0;
        intValue = value.toInt(&okay);

        if (!okay || intValue < 0 || intValue >= dataTypeNames.size())
            return dataTypeNames[0]; //Supposed to be "NULL";
        return dataTypeNames[intValue];
    }
};
