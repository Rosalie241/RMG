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
  public:
    KeyBindButton(QWidget *);
    ~KeyBindButton(void);

  protected:
    void keyPressEvent(QKeyEvent *);
    void mousePressEvent(QMouseEvent *);
};
} // namespace Widget
} // namespace UserInterface

#endif // KEYBINDBUTTON_HPP
