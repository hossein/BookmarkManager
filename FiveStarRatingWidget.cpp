#include "FiveStarRatingWidget.h"

#include <QPainter>
#include <QPixmap>

FiveStarRatingWidget::FiveStarRatingWidget(QWidget *parent) :
    QWidget(parent)
{
    m_starSize = 48;
    m_minimum = 0;
    m_maximum = 100;
    m_value = 50;
}

QSize FiveStarRatingWidget::sizeHint() const
{
    return QSize(m_starSize * 5, m_starSize);
}

void FiveStarRatingWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    QPixmap fiveStarsWhite(":/res/fivestars_white.png");
    p.drawPixmap(0, 0, m_starSize * 5, m_starSize, fiveStarsWhite);

    QPixmap fiveStarsGold(":/res/fivestars.png");
    int realGoldWidth = (fiveStarsGold.width() * 5) * m_value / (m_maximum - m_minimum);
    int starSizeGoldWidth = (m_starSize * 5) * m_value / (m_maximum - m_minimum);
    p.drawPixmap(0, 0, starSizeGoldWidth, m_starSize, fiveStarsGold,
                 0, 0, realGoldWidth, fiveStarsGold.height());

}

void FiveStarRatingWidget::setStarSize(int arg)
{
    if (m_starSize != arg) {
        m_starSize = arg;
        update();
        updateGeometry();
        emit starSizeChanged(arg);
    }
}

void FiveStarRatingWidget::setMinimum(int arg)
{
    if (m_minimum != arg) {
        m_minimum = arg;
        update();
        emit minimumChanged(arg);
    }
}

void FiveStarRatingWidget::setMaximum(int arg)
{
    if (m_maximum != arg) {
        m_maximum = arg;
        update();
        emit maximumChanged(arg);
    }
}

void FiveStarRatingWidget::setValue(int arg)
{
    if (m_value != arg) {
        m_value = arg;
        update();
        emit valueChanged(arg);
    }
}
