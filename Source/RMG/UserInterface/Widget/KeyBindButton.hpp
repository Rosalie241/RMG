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
class KeyBindButton : public QPushButton
{
private:
    QKeySequence keySequence;
    QString currentText;

  public:
    KeyBindButton(QWidget *);
    ~KeyBindButton(void);

  protected:
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void mousePressEvent(QMouseEvent *);
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);
};
} // namespace Widget
} // namespace UserInterface

#endif // KEYBINDBUTTON_HPP
