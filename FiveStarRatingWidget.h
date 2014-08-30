#pragma once
#include <QWidget>

/// Note: Currently this is a view-only widget that is not editable.

class FiveStarRatingWidget : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int starSize READ starSize WRITE setStarSize NOTIFY starSizeChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

    int m_starSize;
    int m_minimum;
    int m_maximum;
    int m_value;

public:
    explicit FiveStarRatingWidget(QWidget *parent = 0);

    // QWidget interface
public:
    virtual QSize sizeHint() const;

protected:
    virtual void paintEvent(QPaintEvent* event);

    // Properties
public:
    int starSize() const { return m_starSize; }
    int minimum() const  { return m_minimum;  }
    int maximum() const  { return m_maximum;  }
    int value() const    { return m_value;    }

signals:
    void starSizeChanged(int arg);
    void minimumChanged(int arg);
    void maximumChanged(int arg);
    void valueChanged(int arg);

public slots:
    void setStarSize(int arg);
    void setMinimum(int arg);
    void setMaximum(int arg);
    void setValue(int arg);
};
