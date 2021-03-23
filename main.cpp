#include "garudadownloader.h"
#if __linux__
#include "updaterwindow.h"
#endif

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile f(":qdarkstyle/style.qss");

    if (!f.exists()) {
      qWarning() << "Unable to set stylesheet, file not found";
    } else {
      f.open(QFile::ReadOnly | QFile::Text);
      QTextStream ts(&f);
      qApp->setStyleSheet(ts.readAll());
    }

    GarudaDownloader w;
    w.show();
#if __linux__
    UpdaterWindow updater(&w);
#endif
    return a.exec();
}
