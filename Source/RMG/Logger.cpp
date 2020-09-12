#include "Logger.hpp"
#include <QDir>

Logger::Logger()
{
}

Logger::~Logger()
{
    this->logfile.flush();
    this->logfile.close();
}

bool Logger::Init()
{
    if (!QDir().exists(APP_LOG_DIR))
    {
        if (!QDir().mkdir(APP_LOG_DIR))
        {
            this->init = false;
            this->error_message = "QDir::mkdir Failed";
            return false;
        }
    }

    this->logfile.setFileName(APP_LOG_FILE);

    if (!this->logfile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        this->init = false;
        this->error_message = "QFile::Open Failed";
        return false;
    }

    this->init = true;
    return true;
}

void Logger::AddText(QString text)
{
    if (!this->init)
        return;

    this->logfile.write(text.toUtf8());
}

QString Logger::GetLastError()
{
    return this->error_message;
}
