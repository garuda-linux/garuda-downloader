#include "garudadownloader.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GarudaDownloader w;
    w.show();
    return a.exec();
}
