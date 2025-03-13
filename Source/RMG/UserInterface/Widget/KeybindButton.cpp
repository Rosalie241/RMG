/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
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

void KeybindButton::Reset(void)
{
    this->setText(this->GetCurrentText());
}

void KeybindButton::Clear(void)
{
    this->SetText("");
}

void KeybindButton::SetText(QString text)
{
    this->currentText = text;
    this->setText(text);
}

void KeybindButton::SetSecondsLeft(int seconds)
{
    if (seconds == 0)
    {
        this->listenForInput = false;
        this->Reset();
    }
    else
    {
        QString text;
        text = "Press key... [";
        text += QString::number(seconds);
        text += "]";
        this->setText(text);
    }
}

QString KeybindButton::GetCurrentText(void)
{
    return this->currentText;
}

void KeybindButton::keyPressEvent(QKeyEvent * event)
{   
    if (!this->listenForInput)
    {
        return;
    }

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
    if (!this->listenForInput)
    {
        return;
    }

    this->currentText = this->keySequence.toString();
    this->clearFocus();
}

void KeybindButton::mousePressEvent(QMouseEvent *event)
{
    if (this->listenForInput)
    {
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        this->currentText = this->text();
        this->listenForInput = true;
        emit this->Clicked(this);
    }
    else if (event->button() == Qt::RightButton)
    {
        this->Clear();
    }
}

void KeybindButton::focusOutEvent(QFocusEvent *event)
{
    if (!this->listenForInput)
    {
        return;
    }

    this->setText(this->currentText);
    emit this->KeybindingChanged(this);
    this->listenForInput = false;
}
