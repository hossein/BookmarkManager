#pragma once
#include <QAbstractItemDelegate>

class AppListItemDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

private:
    static const int SizeHintHeight = 40;

public:
    AppListItemDelegate(QObject* parent = NULL);
    ~AppListItemDelegate();

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
};
