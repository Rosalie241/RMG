#ifndef OGLWIDGET_HPP
#define OGLWIDGET_HPP

#include <QOpenGLWidget>
#include <QOpenGLWindow>
#include <QWidget>
#include <QThread>

namespace UserInterface
{
    namespace Widget
    {
        class OGLWidget : public QOpenGLWindow
        {
        public:
            OGLWidget(void);
            ~OGLWidget(void);

            void SetThread(QThread*);

            QWidget* GetWidget(void);

        protected:
            void exposeEvent(QExposeEvent *) Q_DECL_OVERRIDE;
        };
    } // namespace Widget
} // namespace UserInterface

#endif // OGLWIDGET_HPP