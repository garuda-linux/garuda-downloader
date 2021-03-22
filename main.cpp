#include "garudadownloader.h"
#if __LINUX__
#include "updaterwindow.h"
#endif

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GarudaDownloader w;
    w.show();
#if __LINUX__
    UpdaterWindow updater(&w);
#endif
    return a.exec();
}
