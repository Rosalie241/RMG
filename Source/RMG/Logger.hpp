#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "Config.hpp"
#include <QString>
#include <QFile>

class Logger
{
public:
    Logger();
    ~Logger();

    bool Init();

    void AddText(QString text);
    QString GetLastError();

private:
    bool init;

    QString error_message;
    QFile logfile;
};

#endif // LOGGER_HPP