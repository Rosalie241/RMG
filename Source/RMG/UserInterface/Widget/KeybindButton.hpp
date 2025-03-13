/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef KEYBINDBUTTON_HPP
#define KEYBINDBUTTON_HPP

#include <QPushButton>
#include <QKeySequence>
#include <QKeyEvent>
#include <QMouseEvent>

namespace UserInterface
{
namespace Widget
{
class KeybindButton : public QPushButton
{
  Q_OBJECT

private:
    QKeySequence keySequence;
    QString currentText;
    bool listenForInput = false;

  public:
    KeybindButton(QWidget* parent);
    ~KeybindButton(void);

    void Reset(void);
    void Clear(void);

    void SetText(QString text);
    void SetSecondsLeft(int seconds);

    QString GetCurrentText(void);

  signals:
    void KeybindingChanged(KeybindButton* button);
    void Clicked(KeybindButton* button);

  protected:
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void mousePressEvent(QMouseEvent *);
    void focusOutEvent(QFocusEvent *event);
};
} // namespace Widget
} // namespace UserInterface

#endif // KEYBINDBUTTON_HPP
