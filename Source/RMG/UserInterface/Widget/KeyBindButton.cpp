#include "KeyBindButton.hpp"

using namespace UserInterface::Widget;

KeyBindButton::KeyBindButton(QWidget* parent) : QPushButton(parent)
{

}

KeyBindButton::~KeyBindButton(void)
{

}

void KeyBindButton::keyPressEvent(QKeyEvent * event)
{   
    // filter keys
    switch (event->key())
    {
        // if key is a modifier, only use modifiers()
        case Qt::Key_Control:
        case Qt::Key_Shift:
        case Qt::Key_Alt:
        case Qt::Key_AltGr:
        case Qt::Key_Super_L:
        case Qt::Key_Super_R:
            this->keySequence = QKeySequence(event->modifiers());
            break;
        // if a key requires a modifier(i.e shift), only use key
        case Qt::Key_AsciiTilde:
        case Qt::Key_Exclam:
        case Qt::Key_At:
        case Qt::Key_NumberSign:
        case Qt::Key_Dollar:
        case Qt::Key_Percent:
        case Qt::Key_AsciiCircum:
        case Qt::Key_Ampersand:
        case Qt::Key_Asterisk:
        case Qt::Key_ParenLeft:
        case Qt::Key_ParenRight:
        case Qt::Key_Underscore:
        case Qt::Key_Plus:
        case Qt::Key_BraceLeft:
        case Qt::Key_BraceRight:
        case Qt::Key_Bar:
        case Qt::Key_Colon:
        case Qt::Key_QuoteDbl:
        case Qt::Key_Question:
        case Qt::Key_Greater:
        case Qt::Key_Less:
            this->keySequence = QKeySequence(event->key());
            break;
        default:
            this->keySequence = QKeySequence(event->key() | event->modifiers());
            break;
    }
}

void KeyBindButton::keyReleaseEvent(QKeyEvent *event)
{
    this->currentText = this->keySequence.toString();
    this->clearFocus();
}

void KeyBindButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
    {
        this->setText("");
    }
}

void KeyBindButton::focusInEvent(QFocusEvent *event)
{
    currentText = this->text();
    this->setText("...");
}

void KeyBindButton::focusOutEvent(QFocusEvent *event)
{
    this->setText(this->currentText);
}
