#include "Settings.hpp"

using namespace Utilities;

Settings::Settings(void)
{

}

Settings::~Settings(void)
{
    delete this->settings;
}

bool Settings::Init(void)
{
    this->settings = new QSettings("Config/RMG.ini",  QSettings::IniFormat);

    if (!this->settings->isWritable())
    {
        this->error_Message = "Settings::Init Failed: ";
        this->error_Message += "QSettings is not writable!";
        return false;
    }

    return true;
}

void Settings::SetValue(QString section, QString key, QVariant value)
{
    this->settings->beginGroup(section);
    this->settings->setValue(key, value);
    this->settings->endGroup();
}

QVariant Settings::GetValue(QString section, QString key)
{
    QVariant ret;

    this->settings->beginGroup(section);
    ret = this->settings->value(key);
    this->settings->endGroup();

    return ret;
}

QString Settings::GetLastError(void)
{
    return this->error_Message;
}
