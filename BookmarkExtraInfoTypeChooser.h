#pragma once
#include <QComboBox>

class BookmarkExtraInfoTypeChooser : public QComboBox
{
    Q_OBJECT
public:
    BookmarkExtraInfoTypeChooser(QWidget* parent) : QComboBox(parent)
    {
        //These must correspong to BookmarkManager::BookmarkExtraInfoData::DataType enum values.
        this->addItems(QStringList() << "Null" << "Text" << "Number" << "Boolean");
    }
};
