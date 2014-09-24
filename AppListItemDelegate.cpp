#include "AppListItemDelegate.h"

#include <QFont>
#include <QFontMetrics>
#include <QApplication>
#include <QPainter>

AppListItemDelegate::AppListItemDelegate(QObject* parent)
    : QAbstractItemDelegate(parent)
{

}

AppListItemDelegate::~AppListItemDelegate()
{

}

void AppListItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    //http://www.qtcentre.org/threads/27777-Customize-QListWidgetItem-how-to?p=135369#post135369
    QRect rect = option.rect;
    QPalette palette = QApplication::palette();

    bool isSelected = (option.state & QStyle::State_Selected);
    if (isSelected)
    {
        painter->fillRect(rect, palette.highlight());
    }
    else
    {
        int odd_even_index = index.data(AppItemRole::Index).toInt();
        painter->fillRect(rect, odd_even_index % 2 ? palette.base() : palette.alternateBase());
    }

    QString progName = index.data(AppItemRole::Name).toString();
    QString progPath = index.data(AppItemRole::Path).toString();
    QPixmap progIcon = qvariant_cast<QPixmap>(index.data(AppItemRole::Icon));

    painter->drawPixmap(QRect(rect.x() + 4, rect.y() + 4, 32, 32),
                        progIcon, progIcon.rect());

    //Note: We use rectangles for drawing texts to avoid calculating baseline+-ascent/descent positions.
    QFont boldFont(option.font);
    boldFont.setBold(true);

    QFont boldUnderlinedFont(boldFont);
    boldUnderlinedFont.setUnderline(true);

    QFontMetrics fm(boldFont);
    const int fontHeightPx = fm.height();
    const int topPos = (SizeHintHeight - 2 * fontHeightPx) / 2;
    const int text1Top = rect.top() + topPos;
    const int text2Top = text1Top + fontHeightPx;
    const int textLeft = rect.left() + SizeHintHeight;

    bool isAssociated = index.data(AppItemRole::Assoc).toBool();
    bool isPreferred = index.data(AppItemRole::Pref).toBool();
    //We don't let user un-associate a preferred app in the UI, but anyway we can check with the
    //  following condition:
    //if (isAssociated || isPreferred)
    //... Or with this exact order:
    if (isAssociated)
        painter->setFont(boldFont);
    if (isPreferred)
        painter->setFont(boldUnderlinedFont);

    painter->setPen(QPen(isSelected ? palette.highlightedText() : palette.text(), 1));
    QRect text1rect(textLeft, text1Top, 100000, fontHeightPx);
    painter->drawText(text1rect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, progName);

    painter->setFont(option.font);
    painter->setPen(QPen(isSelected ? palette.highlightedText() : palette.mid(), 1));
    QRect text2rect(textLeft, text2Top, 100000, fontHeightPx);
    painter->drawText(text2rect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, progPath);
}

QSize AppListItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(200, SizeHintHeight);
}
