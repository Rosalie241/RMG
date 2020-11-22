#ifndef OGLWIDGET_HPP
#define OGLWIDGET_HPP

#include <QOpenGLWidget>
#include <QOpenGLWindow>
#include <QResizeEvent>
#include <QThread>
#include <QTimerEvent>
#include <QWidget>

namespace UserInterface
{
namespace Widget
{
class OGLWidget : public QOpenGLWindow
{
  public:
    OGLWidget(QWidget *);
    ~OGLWidget(void);

    void SetThread(QThread *);
    void SetAllowResizing(bool);

    QWidget *GetWidget(void);

  protected:
    void exposeEvent(QExposeEvent *) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *) Q_DECL_OVERRIDE;

  private:
    QWidget *parent;
    bool allowResizing = false;
    int width;
    int height;
    int timerId;
};
} // namespace Widget
} // namespace UserInterface

#endif // OGLWIDGET_HPP