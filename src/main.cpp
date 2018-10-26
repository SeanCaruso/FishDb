#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("SeanSoft");
    QCoreApplication::setApplicationName("FishDb");

    MainWindow w;
    w.show();

    return a.exec();
}
