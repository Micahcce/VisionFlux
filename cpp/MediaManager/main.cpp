#include "QtGUI/MainWindow.h"
#include <QApplication>

using namespace std;


#undef main
int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    MainWindow* window = new MainWindow;
    window->show();

    return a.exec();
}


