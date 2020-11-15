#ifndef EVENTFILTER_HPP
#define EVENTFILTER_HPP

#include <QFocusEvent>
#include <QKeyEvent>
#include <QList>
#include <QObject>

namespace UserInterface
{
class EventFilter : public QObject
{
    Q_OBJECT

  public:
    EventFilter(QObject *);
    ~EventFilter(void);

  protected:
    bool eventFilter(QObject *, QEvent *);

  signals:
    void on_EventFilter_KeyPressed(QKeyEvent *);
    void on_EventFilter_KeyReleased(QKeyEvent *);

    void on_EventFilter_FocusIn(QFocusEvent *);
    void on_EventFilter_FocusOut(QFocusEvent *);
};
} // namespace UserInterface

#endif // EVENTFILTER_HPP