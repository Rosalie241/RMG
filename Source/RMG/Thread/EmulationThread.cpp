#include "EmulationThread.hpp"

using namespace Thread;

EmulationThread::EmulationThread(void)
{
}

EmulationThread::~EmulationThread(void)
{
}

void EmulationThread::SetRomFile(QString file)
{
    this->rom_File = file;   
}

void EmulationThread::run(void)
{
    emit this->on_Emulation_Started();

    bool ret;
    
    ret = g_MupenApi.Core.LaunchEmulation(this->rom_File);

    if (!ret)
        this->error_Message = g_MupenApi.Core.GetLastError();

    emit this->on_Emulation_Finished(ret);
}

QString EmulationThread::GetLastError(void)
{
    return this->error_Message;
}
