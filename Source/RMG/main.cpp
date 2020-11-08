#include <UserInterface/MainWindow.hpp>

#include <QApplication>
#include <QDir>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    UserInterface::MainWindow window;

    QDir::setCurrent(app.applicationDirPath());

    if (!window.Init())
        return 1;

    window.show();

    return app.exec();
}