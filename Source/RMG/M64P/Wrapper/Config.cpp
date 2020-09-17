#include <M64P/Wrapper/Config.hpp>
#include <M64P/Api.hpp>

using namespace M64P::Wrapper;

Config::Config(void)
{
}

Config::~Config(void)
{
}

bool Config::Init()
{
    if (!M64P::Config.IsHooked())
    {
        this->error_Message = "M64P::Config is not hooked!";
        return false;
    }

    return true;
}

bool Config::SetOption(QString section, QString key, int value)
{

}

bool Config::SetOption(QString section, QString key, float value)
{

}

bool Config::SetOption(QString section, QString key, bool value)
{

}

bool Config::SetOption(QString section, QString key, QString value)
{

}

QString Config::GetLastError(void)
{
    return this->error_Message;
}
