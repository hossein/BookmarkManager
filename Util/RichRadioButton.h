#pragma once
#include <QBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVariant>

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(const QString& text, QWidget* parent = Q_NULLPTR) : QLabel(text, parent) { }
    ~ClickableLabel() { }
protected:
    void mousePressEvent(QMouseEvent*) { emit clicked(); }
signals:
    void clicked();
};

class RichRadioButton;
class RichRadioButtonContext
{
public:
    QList<RichRadioButton*> radioButtons;
    QVariant value;
};

class RichRadioButton : public QWidget
{
    Q_OBJECT
private:
    QVariant m_value;
    RichRadioButtonContext* m_context;
    QRadioButton* m_radioButton;

    Q_PROPERTY(bool checked READ isChecked WRITE setChecked)

public:
    RichRadioButton(const QString& text, const QVariant& value, bool checked, RichRadioButtonContext* context, QWidget* parent = Q_NULLPTR)
        : QWidget(parent), m_value(value), m_context(context)
    {
        //Create and set the layout
        m_radioButton = new QRadioButton();
        m_radioButton->setChecked(checked);
        QVBoxLayout* radioLayout = new QVBoxLayout();
        radioLayout->addWidget(m_radioButton, 0, Qt::AlignTop);

        ClickableLabel* label = new ClickableLabel(text);
        label->setWordWrap(true);
        label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        QHBoxLayout* mainLayout = new QHBoxLayout();
        mainLayout->addLayout(radioLayout, 0);
        mainLayout->addWidget(label, 1);
        this->setLayout(mainLayout);

        //Connections
        connect(label, SIGNAL(clicked()), m_radioButton, SLOT(animateClick()));
        connect(m_radioButton, SIGNAL(clicked(bool)), this, SLOT(radioButtonClicked(bool)));

        //Context
        m_context->radioButtons.append(this);
        if (checked)
            m_context->value = value;
    }

    ~RichRadioButton() { }

    bool isChecked() const
    {
        return m_radioButton->isChecked();
    }

public slots:
    void setChecked(bool checked)
    {
        m_radioButton->setChecked(checked);
    }

private slots:
    void radioButtonClicked(bool checked)
    {
        //There is this strange behaviour in Qt, where "the only" radio buttons can be "cleared" by
        //clicking on them:
        //  https://forum.qt.io/topic/6937/solved-forbid-user-to-uncheck-all-qradiobuttons-when-there-is-only-one-button
        //Since all of our radio buttons are alone in their own parent area, user can uncheck them,
        //but we don't want this. So we always check what was clicked, and uncheck the rest.
        Q_UNUSED(checked);
        //  if (!checked)
        //      return;

        foreach (RichRadioButton* rRadio, m_context->radioButtons)
            rRadio->setChecked(rRadio == this);
        m_context->value = m_value;
    }
};
