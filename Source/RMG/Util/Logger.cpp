#include <Util/Logger.hpp>

#include <QDir>

using namespace Util;

Logger::Logger(void)
{
}

Logger::~Logger(void)
{
    if (!this->init_Failed)
    {
        this->logfile.flush();
        this->logfile.close();
    }
}

bool Logger::Init(void)
{
    if (!QDir().exists(APP_LOG_DIR))
    {
        if (!QDir().mkdir(APP_LOG_DIR))
        {
            this->error_message = "QDir::mkdir Failed";
            return false;
        }
    }

    this->logfile.setFileName(APP_LOG_FILE);

    if (!this->logfile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        this->error_message = "QFile::Open Failed";
        return false;
    }

    this->init_Failed = false;
    return true;
}

void Logger::AddText(QString text)
{
    if (this->init_Failed)
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
