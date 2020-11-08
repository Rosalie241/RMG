/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef EMULATIONTHREAD_HPP
#define EMULATIONTHREAD_HPP

//#include "../Globals.hpp"

#include <QString>
#include <QSurfaceFormat>
#include <QThread>

namespace Thread
{
class EmulationThread : public QThread
{
    Q_OBJECT

  public:
    EmulationThread(void);
    ~EmulationThread(void);

    void SetRomFile(QString);

    void run(void) override;

    QString GetLastError(void);

  private:
    QString rom_File;
    QString error_Message;

  signals:
    void on_Emulation_Started(void);
    void on_Emulation_Finished(bool);

    void on_VidExt_SetupOGL(QSurfaceFormat, QThread *);
    void on_VidExt_ResizeWindow(int, int);

    void on_VidExt_Init(void);
    void on_VidExt_SetMode(int, int, int, int, int);
    void on_VidExt_SetModeWithRate(int, int, int, int, int, int);
    void on_VidExt_SetCaption(QString);
    void on_VidExt_ToggleFS(void);
    void on_VidExt_Quit(void);

    void createOGLWindow(QSurfaceFormat *format, QThread *thread);
    void resizeMainWindow(int Width, int Height);
};
} // namespace Thread

#endif // EMULATIONTHREAD_HPP
