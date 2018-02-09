#include "mainwin.h"
#include <QApplication>
#include "palettem.h"

int main(int argc, char *argv[])
{
    make_tables();
    QApplication a(argc, argv);
    MainWin w;
    w.show();
    return a.exec();
}
