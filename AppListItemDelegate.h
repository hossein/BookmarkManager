#pragma once
#include <QAbstractItemDelegate>

struct AppItemRole
{
    enum AppItemRoleEnum
    {
        Icon = Qt::DecorationRole,
        Name = Qt::DisplayRole,
        SAID = Qt::UserRole + 0,
        Path = Qt::UserRole + 1,
        Index= Qt::UserRole + 2,
        Assoc= Qt::UserRole + 3,
        Pref = Qt::UserRole + 4
    };
};

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
