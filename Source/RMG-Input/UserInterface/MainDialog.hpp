/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef MAINDIALOG_HPP
#define MAINDIALOG_HPP

#include <QDialog>

#include "ui_MainDialog.h"
#include "Widget/ControllerWidget.hpp"
#include "Thread/SDLThread.hpp"
#include "EventFilter.hpp"

namespace UserInterface
{
class MainDialog : public QDialog, private Ui::MainDialog
{
Q_OBJECT
private:
    struct inputDevice_t
    {
        QString deviceName;
        int deviceNum;

        bool operator== (inputDevice_t other) const
        {
            return other.deviceNum == deviceNum &&
                other.deviceName == deviceName;
        }
    };

    QTimer* inputPollTimer;
    Thread::SDLThread* sdlThread;

    QList<inputDevice_t> oldInputDeviceList;
    QList<inputDevice_t> inputDeviceList;
    bool updatingDeviceList = false;

    QList<Widget::ControllerWidget*> controllerWidgets;
    SDL_Joystick* currentJoystick         = nullptr;
    SDL_GameController* currentController = nullptr;
    QString currentDeviceName;
    int currentDeviceNum = 0;

    EventFilter* eventFilter;

    void addInputDevice(QString, int);
    void removeInputDevice(QString, int);

    void openInputDevice(QString, int);
    void closeInputDevice();

public:
    MainDialog(QWidget *parent, Thread::SDLThread*);
    ~MainDialog(void);

public slots:
    void on_InputPollTimer_triggered();

    void on_ControllerWidget_CurrentInputDeviceChanged(ControllerWidget*, QString, int);
    void on_ControllerWidget_RefreshInputDevicesButtonClicked();
   
    void on_tabWidget_currentChanged(int);

    void on_SDLThread_DeviceFound(QString, int);
    void on_SDLThread_DeviceSearchFinished(void);

private slots:
    void on_EventFilter_KeyPressed(QKeyEvent *);
    void on_EventFilter_KeyReleased(QKeyEvent *);

    void on_buttonBox_clicked(QAbstractButton *);
};
}

#endif // MAINDIALOG_HPP
