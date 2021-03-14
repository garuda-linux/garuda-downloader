#include "garudadownloader.h"
#include "./ui_garudadownloader.h"

// Zsync
#include <zsclient.h>

#include <QtConcurrent/QtConcurrent>
#include <QFile>
#include <QFileDialog>
#include <QDir>

// TODO: Don't hardcode this
const QString url = "https://builds.garudalinux.org/iso/garuda/%1/210313/garuda-%1-linux-zen-210313.iso.zsync";
QDir dir("Garuda Downlaoder");

void ZSyncDownloader::run() {

    emit done(client->run());
}

GarudaDownloader::GarudaDownloader(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GarudaDownloader)
{
    ui->setupUi(this);
    setButtonStates(false);
    connect(&zsync_updatetimer, SIGNAL(timeout()), this, SLOT(onUpdate()));
}

GarudaDownloader::~GarudaDownloader()
{
    if (zsync_client)
        zsync_downloader->terminate();
    delete ui;
}

void GarudaDownloader::on_downloadButton_clicked()
{
    if (!zsync_client)
    {
        if (!dir.exists())
            if (!QDir().mkdir(dir.absolutePath()))
                return;


        QString edition = this->ui->comboBox->currentText().toLower();
        zsync_client = new zsync2::ZSyncClient(url.arg(edition).toStdString(), "current.iso");
        zsync_client->setCwd(dir.absolutePath().toStdString());
        zsync_client->setRangesOptimizationThreshold(64 * 4096);
        if (!seed_file.isEmpty() && QFile::exists(seed_file))
            zsync_client->addSeedFile(seed_file.toStdString());

        zsync_downloader = new ZSyncDownloader(zsync_client);
        connect(zsync_downloader, &ZSyncDownloader::done, this, &GarudaDownloader::onDownloadFinished);
        connect(zsync_downloader, &ZSyncDownloader::finished, this, &GarudaDownloader::onDownlaodStop);
        zsync_downloader->start();

        this->ui->downloadButton->setText("Cancel");
        this->ui->statusText->setText("Initializing");
        setButtonStates(true);

        finished = false;
        zsync_updatetimer.start(500);
    }
    else
    {
        zsync_downloader->terminate();
        zsync_updatetimer.stop();
    }
}

void GarudaDownloader::onDownloadFinished(bool success)
{
    zsync_updatetimer.stop();
    finished = true;

    if (success)
    {
        this->ui->progressBar->setValue(100);
        this->ui->statusText->setText("Download finished!");
    }
    else {
        this->ui->progressBar->setValue(100);
        std::string out;
        if (zsync_client->nextStatusMessage(out))
        {
            while (zsync_client->nextStatusMessage(out));
            this->ui->statusText->setText(QString::fromStdString(out));
        }
    }

    QApplication::alert(this);
}

void GarudaDownloader::onEtcherDownloadFinished(bool success)
{
    zsync_updatetimer.stop();
    finished = true;

    if (success)
    {
        this->ui->progressBar->setValue(0);
        this->ui->statusText->setText("Idle");
        this->hide();
        QFile::setPermissions(dir.absoluteFilePath("./etcher.AppImage"), QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
        QProcess::execute(dir.absoluteFilePath("etcher.AppImage"), { dir.absoluteFilePath("./current.iso") });
        this->show();
    }
    else {
        this->ui->progressBar->setValue(100);
        std::string out;
        if (zsync_client->nextStatusMessage(out))
        {
            while (zsync_client->nextStatusMessage(out));
            this->ui->statusText->setText(QString::fromStdString(out));
        }
    }

    QApplication::alert(this);
}

void GarudaDownloader::onDownlaodStop()
{
    this->ui->downloadButton->setText("Download");
    if (!finished)
    {
        this->ui->progressBar->setValue(0);
        this->ui->statusText->setText("Idle");
    }

    setButtonStates(false);

    delete zsync_client;
    zsync_client = nullptr;
    zsync_downloader->deleteLater();

    // Clean up some files...
    QDir wildcards(dir.absolutePath());
    wildcards.setNameFilters({"*.zs-old", "rcksum-*"});
    for(const QString & filename: wildcards.entryList()){
        dir.remove(filename);
    }
}

void GarudaDownloader::setButtonStates(bool downloading)
{
    this->ui->selectButton->setDisabled(downloading);
#if __linux__
    this->ui->flashButton->setDisabled(downloading ? true : (!QFile::exists(dir.absoluteFilePath("current.iso"))));
#endif
#if _WIN32
    // TODO
#endif
}

void GarudaDownloader::onUpdate()
{
    this->ui->progressBar->setValue(zsync_client->progress() * 100);
    std::string out;
    if (zsync_client->nextStatusMessage(out))
    {
        while (zsync_client->nextStatusMessage(out));
        this->ui->statusText->setText(QString::fromStdString(out));
    }
}

void GarudaDownloader::on_selectButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("ISO Files(*.iso)"));
    if (dialog.exec())
    {
        seed_file = dialog.selectedFiles()[0];
        this->ui->selectButton->setText("Seed file: " + QFileInfo(seed_file).fileName());
    }
}

void GarudaDownloader::on_flashButton_clicked()
{
    if (!zsync_client)
    {
        QTemporaryFile file;
        if (file.open())
        {
            // This is absolutely terrible don't ever do this
            file.close();
            auto path = file.fileName();
            file.remove();
            QFile::copy(":/main/resources/etcher.zsync", path);

            zsync_client = new zsync2::ZSyncClient(path.toStdString(), "etcher.AppImage");
            zsync_client->setCwd(dir.absolutePath().toStdString());
            zsync_client->setRangesOptimizationThreshold(64 * 4096);
            zsync_downloader = new ZSyncDownloader(zsync_client);
            connect(zsync_downloader, &ZSyncDownloader::done, this, &GarudaDownloader::onEtcherDownloadFinished);
            connect(zsync_downloader, &ZSyncDownloader::finished, this, &GarudaDownloader::onDownlaodStop);
            zsync_downloader->start();

            this->ui->downloadButton->setText("Cancel");
            this->ui->statusText->setText("Initializing");
            setButtonStates(true);

            finished = false;
            zsync_updatetimer.start(500);
        }
    }
}
