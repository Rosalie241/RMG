#include <Util/Logger.hpp>

#include <QDir>

Logger g_Logger;

Logger::Logger()
{
}

Logger::~Logger()
{
    if (this->init)
    {
        this->logfile.flush();
        this->logfile.close();
    }
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

#include <iostream>
void Logger::AddText(QString text)
{
    if (!this->init)
        return;

    if (text[text.size() - 1] != "\n")
        text += "\n";

    this->logfile.write(text.toUtf8());
    this->logfile.flush();
}

QString Logger::GetLastError()
{
    return this->error_message;
}
