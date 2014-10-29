#pragma once
#include <QItemDelegate>
#include <QComboBox>

class BookmarkExtraInfoTypeChooser : public QItemDelegate
{
    Q_OBJECT

public:
    BookmarkExtraInfoTypeChooser(QWidget* parent) : QItemDelegate(parent)
    {

    }

    // QAbstractItemDelegate interface
    QWidget *createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        Q_UNUSED(option)
        Q_UNUSED(index)

        //These must correspong to BookmarkManager::BookmarkExtraInfoData::DataType enum values.
        QComboBox* cboItems = new QComboBox(parent);
        cboItems->addItems(QStringList() << "Null" << "Text" << "Number" << "Boolean");

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
};
