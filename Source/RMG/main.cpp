#include <UserInterface/MainWindow.hpp>

#include <QApplication>
#include <QDir>
#include <QFile>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    UserInterface::MainWindow window;

    QDir::setCurrent(app.applicationDirPath());

    if (!window.Init())
    {
        return 1;
    }

    window.show();

    // try to find an argument
    // with a file that exists,
    // if such an argument exists,
    // attempt to open that ROM
    for (int i = 1; i < argc; i++)
    {
        QString file = argv[i];
        if (QFile().exists(file))
        {
            window.OpenROM(file);
            break;
        }
    }

    return app.exec();
}