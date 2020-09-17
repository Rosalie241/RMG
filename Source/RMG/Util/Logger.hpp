#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <Config.hpp>

#include <QString>
#include <QFile>

namespace Util
{
    class Logger
    {
    public:
        Logger(void);
        ~Logger(void);

        bool Init(void);

        void AddText(QString);

        QString GetLastError(void);

    private:
        bool init_Failed = true;

        QString error_message;
        QFile logfile;
    };

} // namespace Util

extern Util::Logger g_Logger;

#endif // LOGGER_HPP