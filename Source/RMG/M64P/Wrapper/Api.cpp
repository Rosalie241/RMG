#include "Api.hpp"
#include "../Api.hpp"
#include "../Macros.hpp"

using namespace M64P::Wrapper;

Api::Api(void)
{
}

Api::~Api(void)
{
    this->core_Handle_Close();
}

bool Api::Init(QString file)
{
    this->error_Message = "Api::Init Failed: ";

    if (!this->core_Handle_Open(file))
        return false;

    if (!M64P::Core.Hook(this->core_Handle))
    {
        this->error_Message += M64P::Core.GetLastError();
        return false;
    }

    if (!M64P::Config.Hook(this->core_Handle))
    {
        this->error_Message += M64P::Config.GetLastError();
        return false;
    }

    if (!this->Core.Init(this->core_Handle))
    {
        this->error_Message += this->Core.GetLastError();
        return false;
    }

    if (!this->Config.Init())
    {
        this->error_Message += this->Config.GetLastError();
        return false;
    }

    return true;
}

QString Api::GetLastError(void)
{
    return this->error_Message;
}

bool Api::core_Handle_Open(QString file)
{
    this->core_Handle_Close();

    this->core_Handle = DLOPEN(file.toStdString().c_str());
    if (this->core_Handle == NULL)
    {
        this->error_Message += DLGETERRSTR();
        return false;
    }

    this->core_Handle_Opened = true;

    return true;
}

void Api::core_Handle_Close(void)
{
    if (!this->core_Handle_Opened)
        return;

    DLCLOSE(this->core_Handle);
    this->core_Handle_Opened = false;
}
