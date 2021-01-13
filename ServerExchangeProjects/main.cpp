#include "mainwindow.h"

#include <QApplication>

const int port = 7654;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.startServer(7654);
    w.connectToDB();
    w.show();
    return a.exec();
}
