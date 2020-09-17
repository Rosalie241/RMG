#ifndef M64P_WRAPPER_CONFIG_HPP
#define M64P_WRAPPER_CONFIG_HPP

#include <M64P/ConfigApi.hpp>

#include <QString>

namespace M64P
{
    namespace Wrapper
    {
        class Config
        {
        public:
            Config(void);
            ~Config(void);

            bool Init();

            bool SetOption(QString section, QString key, int value);
            bool SetOption(QString section, QString key, float value);
            bool SetOption(QString section, QString key, bool value);
            bool SetOption(QString section, QString key, QString value);

            QString GetLastError(void);

        private:
            QString error_Message;
        };
    } // namespace Wrapper
} // namespace M64P

#endif // M64P_WRAPPER_CONFIG_HPP
