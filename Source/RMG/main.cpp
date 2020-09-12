#include <QApplication>
#include <QDir>

#include "UserInterface/MainWindow.hpp"
int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    MainWindow window;

    QDir::setCurrent(app.applicationDirPath());

    if (!window.Init())
        return 1;
        
    window.show();

    return app.exec();
}