#ifndef OGLWIDGET_HPP
#define OGLWIDGET_HPP

#include <QOpenGLWidget>
#include <QOpenGLWindow>
#include <QThread>
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

    QWidget *GetWidget(void);

  protected:
    void exposeEvent(QExposeEvent *) Q_DECL_OVERRIDE;

  private:
    QWidget *parent;
};
} // namespace Widget
} // namespace UserInterface

#endif // OGLWIDGET_HPP