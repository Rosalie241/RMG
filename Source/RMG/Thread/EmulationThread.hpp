#ifndef EMULATIONTHREAD_HPP
#define EMULATIONTHREAD_HPP

#include "../Globals.hpp"

#include <QThread>
#include <QString>

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
    };
} // namespace Thread

#endif // EMULATIONTHREAD_HPP
