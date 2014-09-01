#include "FiveStarRatingWidget.h"

#include <QPainter>
#include <QPixmap>

FiveStarRatingWidget::FiveStarRatingWidget(QWidget *parent) :
    QWidget(parent)
{
    m_starSize = 48; //CONST
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

void FiveStarRatingWidget::setStarSize(int size)
{
    if (m_starSize != size) {
        m_starSize = size;
        update();
        updateGeometry();
        emit starSizeChanged(size);
    }
}

void FiveStarRatingWidget::setMinimum(int min)
{
    if (m_minimum != min) {
        m_minimum = min;
        update();
        emit minimumChanged(min);
    }
}

void FiveStarRatingWidget::setMaximum(int max)
{
    if (m_maximum != max) {
        m_maximum = max;
        update();
        emit maximumChanged(max);
    }
}

void FiveStarRatingWidget::setValue(int value)
{
    if (m_value != value) {
        m_value = value;
        update();
        emit valueChanged(value);
    }
}
