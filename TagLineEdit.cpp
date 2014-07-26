#include "TagLineEdit.h"

#include <QDebug>
#include <QKeyEvent>
#include <QListWidget>
#include <QAbstractItemModel>

TagLineEdit::TagLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    m_model = NULL;

    lwPopup = new QListWidget(this);
    lwPopup->hide();
    lwPopup->setWindowFlags(Qt::ToolTip);
    lwPopup->setSelectionMode(QAbstractItemView::SingleSelection);
}

void TagLineEdit::setModel(QAbstractItemModel* model)
{
    m_model = model;
}

void TagLineEdit::setModelColumn(int column)
{
    m_modelcolumn = column;
}

void TagLineEdit::keyPressEvent(QKeyEvent* event)
{
    if (m_model == NULL)
        return;

    bool requestComplete = false;
    bool dontProcess = false;

    switch (event->key())
    {
    case Qt::Key_Left:
    case Qt::Key_Right:
        //Turn off the completer
        QLineEdit::keyPressEvent(event);
        lwPopup->hide();
        return;

    case Qt::Key_Up:
        if (lwPopup->isVisible())
        {
            lwPopup->clearSelection();
            if (lwPopup->currentRow() == lwPopup->count() - 1)
                lwPopup->setCurrentRow(0, QItemSelectionModel::Select);
            else
                lwPopup->setCurrentRow(lwPopup->currentRow() + 1);
        }
        else
            QLineEdit::keyPressEvent(event);
        return;
    case Qt::Key_Down:
        if (lwPopup->isVisible())
        {
            lwPopup->clearSelection();
            if (lwPopup->currentRow() == 0)
                lwPopup->setCurrentRow(lwPopup->count() - 1, QItemSelectionModel::Select);
            else
                lwPopup->setCurrentRow(lwPopup->currentRow() - 1);
        }
        else
            QLineEdit::keyPressEvent(event);
        return;

    case Qt::Key_Space:
        if (event->modifiers() == Qt::ControlModifier)
            requestComplete = false; //Show the listwidget
        else if (lwPopup->isVisible())
            requestComplete = true;
        break;

    //NOTE: The following two are not directly received, we pass them from the `event()` function.
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        if (lwPopup->isVisible())
        {
            requestComplete = true;
            dontProcess = true;
            break;
        }
        else
        {
            event->ignore();
            return;
        }

    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (lwPopup->isVisible())
        {
            requestComplete = true;
            dontProcess = true;
        }
        break;

    case Qt::Key_Escape:
        if (lwPopup->isVisible())
            lwPopup->hide();
        else
            QLineEdit::keyPressEvent(event);
        return;
    }

    if (!requestComplete)
        QLineEdit::keyPressEvent(event);

    QString thisText = this->text();
    int pos1 = this->cursorPosition(),
        pos2 = this->cursorPosition();

    //We wrap the `for`s in ifs to avoid increasing/decreasing the positions if
    //  the cursor is already on space.
    if (pos1 >0 && thisText[pos1-1] != ' ')
    {
        for ( ; pos1 > 0; pos1--)
        {
            if (thisText[pos1] == ' ')
            {
                pos1++;
                break;
            }
        }
    }

    if (thisText[pos2] != ' ')
    {
        for ( ; pos2 < thisText.length(); pos2++)
        {
            if (thisText[pos2] == ' ')
            {
                pos2--;
                break;
            }
        }
    }

    if (!requestComplete)
    {
        QString textpresent = thisText.mid(pos1, pos2 - pos1);
        if (textpresent.length() == 0)
        {
            lwPopup->hide();
            return;
        }

        QModelIndexList matches =
            m_model->match(m_model->index(0, m_modelcolumn), Qt::DisplayRole, textpresent, -1,
                           Qt::MatchContains);

        if (matches.length() == 0)
        {
            lwPopup->hide();
            return;
        }

        lwPopup->clear();
        foreach (QModelIndex index, matches)
            lwPopup->addItem(m_model->data(index).toString());
        lwPopup->setCurrentRow(0, QItemSelectionModel::Select);



        bool hasSelection = this->hasSelectedText();
        int originalSelStart = this->selectionStart();
        int originalCursorPos = this->cursorPosition();

        this->setCursorPosition(pos1);
        int startOfWordX = this->cursorRect().left();

        if (hasSelection)
            this->setSelection(originalSelStart, originalCursorPos - originalSelStart);
        else
            this->setCursorPosition(originalCursorPos);



        QPoint lwPopupOffset(startOfWordX, this->height());
        lwPopup->move(this->parentWidget()->mapToGlobal(this->pos()) + lwPopupOffset);
        //qDebug() << this->pos() << lwPopup->pos();
        lwPopup->resize(80, 120);
        lwPopup->show();
    }
    else
    {
        //Do NOT do NOW: QLineEdit::keyPressEvent(event);
        //We should only do it for space; after we're done.

        QString finalText = thisText.mid(0, pos1);
        finalText += lwPopup->currentIndex().data().toString();
        int cursorPos = finalText.length();
        finalText += thisText.mid(pos2);

        this->setText(finalText);
        this->setCursorPosition(cursorPos);
        //this->setSelection(cursorPos, 0);

        lwPopup->hide();

        if (dontProcess)
            event->accept();
        if (!dontProcess)
        {
            QLineEdit::keyPressEvent(event);
        }
    }

}

void TagLineEdit::focusOutEvent(QFocusEvent* event)
{
    lwPopup->hide();
    QLineEdit::focusOutEvent(event);
}

bool TagLineEdit::event(QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab)
        {
            keyPressEvent(ke);
            if (event->isAccepted())
                return true;
            else
                return QLineEdit::event(event);
        }
        else
            return QLineEdit::event(event);
    }
    return QLineEdit::event(event);
}
