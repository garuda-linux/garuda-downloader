#include "updaterwindow.h"
#include "ui_updaterwindow.h"
#include "garudadownloader.h"
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QPropertyAnimation>
#include <QSettings>
#include <QSysInfo>
#include <iostream>

void UpdaterWindow::doUpdate(QWidget *parent) {
  ui->progressBar->setValue(100);
  QMessageBox::StandardButton reply = QMessageBox::question(
      this, "Update", "Garuda Downloader update found! Do you want to update?",
      QMessageBox::Yes | QMessageBox::No);
  if (reply == QMessageBox::Yes) {
    parent->hide();
    this->show();

    ui->label->setText("Updating...");
    ui->progressBar->setValue(0);
    auto updater = new QProcess();
    updater->setProcessChannelMode(QProcess::MergedChannels);
    updater->setWorkingDirectory(qApp->applicationDirPath() +
                                 "/appimageupdatetool");
    updater->start("./AppRun", {"-O", "-r", qgetenv("APPIMAGE")});

    if (updater->waitForStarted()) {
      connect(updater, &QProcess::readyReadStandardOutput, [=]() {
        QString data = updater->readAllStandardOutput();
        std::cout << data.toStdString() << std::endl;
        bool ok;
        int percent = data.remove(0, 5).split("%")[0].toFloat(&ok);
        if (ok)
          ui->progressBar->setValue(percent);
      });
      connect(updater,
              QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
              [=](int exitCode) {
                if (exitCode != 0)
                  QMessageBox::critical(this, "Update failed!",
                                        "Update failed!");
                updater->deleteLater();
                // Restart the program
                QProcess restart;
                restart.startDetached(qgetenv("APPIMAGE"));
                qApp->quit();
              });
    } else {
      updater->kill();
      updater->deleteLater();
    }
  }
}

UpdaterWindow::UpdaterWindow(QWidget *parent)
    : QDialog(parent), ui(new Ui::updaterwindow) {
  ui->setupUi(this);
  auto updatecheck = new QProcess();
  updatecheck->setWorkingDirectory(qApp->applicationDirPath() +
                                   "/appimageupdatetool");
  updatecheck->start("./AppRun", {"-j", qgetenv("APPIMAGE")});
  if (updatecheck->waitForStarted(2000)) {
    auto animation = new QPropertyAnimation(ui->progressBar, "value");
    animation->setDuration(8000);
    animation->setStartValue(0);
    animation->setEndValue(100);
    animation->start();
    connect(animation, &QPropertyAnimation::finished,
            [=]() { updatecheck->kill(); });

    connect(updatecheck,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
              animation->stop();
              animation->deleteLater();
              updatecheck->deleteLater();
              if (exitCode == 1 && exitStatus == QProcess::NormalExit)
                doUpdate(parent);
            });
  } else {
    updatecheck->kill();
    delete updatecheck;
  }
}

UpdaterWindow::~UpdaterWindow() {
  delete ui;
}
