#include "garudadownloader.h"
#include "updaterwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GarudaDownloader w;
    w.show();
    UpdaterWindow updater(&w);
    return a.exec();
}
