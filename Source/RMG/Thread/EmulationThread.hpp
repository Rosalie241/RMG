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

#ifndef _WIN32
#include <QDBusInterface>
#endif
#include <QSurfaceFormat>
#include <QString>
#include <QThread>

enum class VidExtRenderMode
{
    OpenGL  = 0,
    Vulkan  = 1,
    Invalid = 2
};

namespace Thread
{
class EmulationThread : public QThread
{
    Q_OBJECT

  public:
    EmulationThread(QObject *);
    ~EmulationThread(void);

    void SetRomFile(QString);
    void SetDiskFile(QString);
    void SetNetplay(QString address, int port, int player);

    void run(void) override;

  private:
    QString rom;
    QString disk;
    QString address;
    int port   = -1;
    int player = -1;
#ifndef _WIN32
    uint32_t dbusCookieId = 0;
    QDBusInterface* dbusInterface = nullptr;
#endif

    void resetState(void);
    void inhibitScreensaver(void);
    void uninhibitScreensaver(void);

  signals:
    void on_Emulation_Started(void);
    void on_Emulation_Finished(bool ret, QString error);

    void on_VidExt_SetupOGL(QSurfaceFormat, QThread *);
    void on_VidExt_ResizeWindow(int, int);

    void on_VidExt_Init(VidExtRenderMode);
    void on_VidExt_SetWindowedMode(int, int, int, int);
    void on_VidExt_SetFullscreenMode(int, int, int, int);
    void on_VidExt_SetCaption(QString);
    void on_VidExt_ToggleFS(bool);
    void on_VidExt_Quit(void);
};
} // namespace Thread

#endif // EMULATIONTHREAD_HPP
