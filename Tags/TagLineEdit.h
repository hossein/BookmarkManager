#pragma once
#include <QLineEdit>

class QAbstractItemModel;
class QListWidget;

class TagLineEdit : public QLineEdit
{
    Q_OBJECT

private:
    QListWidget* lwPopup;
    QAbstractItemModel* m_model;
    int m_modelcolumn;

public:
    explicit TagLineEdit(QWidget *parent = 0);

public slots:
    /// The TagLineEdit does not get ownership of the model.
    /// The same model can be used on multiple TagLineEdits.
    void setModel(QAbstractItemModel* model);
    void setModelColumn(int column);

protected:
    void keyPressEvent(QKeyEvent* event);




    // QWidget interface
protected:
    void focusOutEvent(QFocusEvent*event);

    // QObject interface
public:
    bool event(QEvent* event);
};
