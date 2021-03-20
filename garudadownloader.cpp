#include "garudadownloader.h"
#include "./ui_garudadownloader.h"

// Zsync
#include <zsclient.h>

#include <QtConcurrent/QtConcurrent>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QDesktopServices>

// TODO: Don't hardcode this
const QString url = "https://builds.garudalinux.org/iso/latest/garuda/%1/latest.iso.zsync";

auto download_dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
QDir dir(download_dir.isEmpty() ? "Garuda Downloader" : QDir(download_dir).filePath("Garuda Downloader"));

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
        this->ui->statusText->setText("Download finished! Location: <a href=\"#open_folder\">" + dir.absolutePath() + "</a>");
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

void GarudaDownloader::updateSelectSize()
{
    if (seed_file.isEmpty())
        return;
    // https://gist.github.com/andrey-str/0f9c7709cbf0c9c49ef9
    QFontMetrics metrix(ui->selectButton->font());
    int width = ui->selectButton->width() - 2;
    QString clippedText = metrix.elidedText("Seed file: " + QFileInfo(seed_file).fileName(), Qt::ElideMiddle, width);
    ui->selectButton->setText(clippedText);
}

void GarudaDownloader::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateSelectSize();
    this->setMaximumHeight(this->sizeHint().height());
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
    while (zsync_client->nextStatusMessage(out))
    {
        qInfo() << QString::fromStdString(out);
        if (out.rfind("optimized ranges,", 0) == 0)
            continue;
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
        // Apply text
        updateSelectSize();
    }
}

void GarudaDownloader::on_flashButton_clicked()
{
    if (!zsync_client)
    {
        if (QFile::exists("/usr/bin/balena-etcher-electron"))
        {
            this->hide();
            QProcess::execute("/usr/bin/balena-etcher-electron", { dir.absoluteFilePath("./current.iso") });
            this->show();
            return;
        }

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

void GarudaDownloader::on_statusText_linkActivated(const QString &link)
{
    if (link == "#open_folder")
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}
