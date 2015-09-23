#pragma once
#include <QListWidget>
#include <QPainter>
#include <QDebug>
class ListWidgetWithEmptyPlaceholder : public QListWidget
{
    Q_OBJECT

    QString m_placeholderText;
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)

public:
    ListWidgetWithEmptyPlaceholder(QWidget* parent = NULL) : QListWidget(parent)
    {
        m_placeholderText = "(empty)";
    }

    ~ListWidgetWithEmptyPlaceholder() { }

protected:
    virtual void paintEvent(QPaintEvent* event)
    {
        //From http://stackoverflow.com/questions/20765547/qlistview-show-text-when-list-is-empty
        QListWidget::paintEvent(event);

        //if (model() && model()->rowCount(rootIndex()))
        //    return;
        int visibleItems = 0;
        for (int i = 0; i < this->count(); i++)
            if (!this->item(i)->isHidden())
                visibleItems += 1;
        if (visibleItems > 0)
            return;

        //Empty, draw placeholder text:
        QPainter p(this->viewport()); //For centering to be correct
        QFont boldFont = this->font();
        boldFont.setBold(true);

        p.setPen(QColor::fromRgb(128, 128, 128));
        p.setFont(boldFont);
        p.drawText(rect(), Qt::AlignCenter, m_placeholderText);
    }

public:
    QString placeholderText() const
    {
        return m_placeholderText;
    }

public slots:
    void setPlaceholderText(QString text)
    {
        m_placeholderText = text;
    }
};
