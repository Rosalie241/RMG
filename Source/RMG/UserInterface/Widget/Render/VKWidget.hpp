/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef VKWIDGET_HPP
#define VKWIDGET_HPP

#include <QThread>
#include <QResizeEvent>
#include <QWindow>
#include <QTimerEvent>
#include <QWidget>

namespace UserInterface
{
namespace Widget
{
class VKWidget : public QWindow
{
  public:
    VKWidget(QWidget *);
    ~VKWidget(void);

    void SetHideCursor(bool hide);

    QWidget* GetWidget(void);

    void SetActive(bool value);

  protected:
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *) Q_DECL_OVERRIDE;

  private:
    QWidget* widgetContainer = nullptr;
    int width                = 0;
    int height               = 0;
    int timerId              = 0;
};
} // namespace Widget
} // namespace UserInterface

#endif // VKWIDGET_HPP