#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QSettings>

namespace Utilities
{
    class Settings
    {
    public:
        Settings(void);
        ~Settings(void);

        bool Init(void);

        void SetValue(QString, QString, QVariant);
        QVariant GetValue(QString, QString);

        QString GetLastError(void);
    private:
        QString error_Message;

        QSettings* settings;
    };
} // namespace Utilities

#endif // SETTINGS_HPP
