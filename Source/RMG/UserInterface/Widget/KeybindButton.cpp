/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "KeybindButton.hpp"

using namespace UserInterface::Widget;

KeybindButton::KeybindButton(QWidget* parent) : QPushButton(parent)
{
}

KeybindButton::~KeybindButton(void)
{
}

void KeybindButton::Clear(void)
{
    this->setText("");
    emit this->on_KeybindButton_KeybindingChanged(this);
}

void KeybindButton::keyPressEvent(QKeyEvent * event)
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

void KeybindButton::keyReleaseEvent(QKeyEvent *event)
{
    this->currentText = this->keySequence.toString();
    this->clearFocus();
}

void KeybindButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
    {
        this->Clear();
    }
}

void KeybindButton::focusInEvent(QFocusEvent *event)
{
    currentText = this->text();
    this->setText("...");
}

void KeybindButton::focusOutEvent(QFocusEvent *event)
{
    this->setText(this->currentText);
    emit this->on_KeybindButton_KeybindingChanged(this);
}
