#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <Config.hpp>

#include <QString>
#include <QFile>

class Logger
{
public:
    Logger();
    ~Logger();

    bool Init(void);

    void AddText(QString text);

    QString GetLastError(void);

private:
    bool init;

    QString error_message;
    QFile logfile;
};

extern Logger g_Logger;

#endif // LOGGER_HPP