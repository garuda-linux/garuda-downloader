#include "garudadownloader.h"
#include "./ui_garudadownloader.h"

// Zsync
#if __UNIX__
#include <zsclient.h>
#else
#include <windows.h>
#endif

#include <QtConcurrent/QtConcurrent>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QDesktopServices>
#include <functional>

#if __UNIX__
auto download_dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
QDir dir(download_dir.isEmpty() ? "Garuda Downloader" : QDir(download_dir).filePath("Garuda Downloader"));
#else
QDir dir("Garuda Downloader");
#endif

#if __UNIX__
void ZSyncDownloader::run() {

    emit done(client->run());
}
#endif

GarudaDownloader::GarudaDownloader(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GarudaDownloader)
{
    ui->setupUi(this);
    setButtonStates(false);
    connect(&zsync_updatetimer, SIGNAL(timeout()), this, SLOT(onUpdate()));
#if __WIN32
    ui->flashButton->setText("Launch Rufus");
#endif
}

GarudaDownloader::~GarudaDownloader()
{
#if __UNIX__
    if (zsync_client)
        zsync_downloader->terminate();
#else
    if (zsync_windows_downloader)
        zsync_windows_downloader->kill();
#endif
    delete ui;
}

void GarudaDownloader::on_downloadButton_clicked()
{
#if __UNIX__
    if (!zsync_client)
#else
    if (!zsync_windows_downloader)
#endif
    {
        if (!dir.exists())
            if (!QDir().mkdir(dir.absolutePath()))
                return;


        QString edition = this->ui->comboBox->currentText().toLower();
#if __UNIX__
        zsync_client = new zsync2::ZSyncClient(QString("https://mirrors.fossho.st/garuda/iso/latest/garuda/%1/latest.iso.zsync").arg(edition).toStdString(), "current.iso");
        zsync_client->setCwd(dir.absolutePath().toStdString());
        zsync_client->setRangesOptimizationThreshold(64 * 4096);
        if (!seed_file.isEmpty() && QFile::exists(seed_file))
            zsync_client->addSeedFile(seed_file.toStdString());

        zsync_downloader = new ZSyncDownloader(zsync_client);
        connect(zsync_downloader, &ZSyncDownloader::done, this, &GarudaDownloader::onDownloadFinished);
        connect(zsync_downloader, &ZSyncDownloader::finished, this, &GarudaDownloader::onDownloadStop);
        zsync_downloader->start();
#else
        zsync_windows_downloader = new QProcess();
        // Has to be http, zsync here has no access to libcurl certs
        QStringList arguments = {QString("http://mirrors.fossho.st/garuda/iso/latest/garuda/%1/latest.iso.zsync").arg(edition), "-o", "current.iso", "--force-update"};
        zsync_windows_downloader->setProcessChannelMode(QProcess::MergedChannels);
        zsync_windows_downloader->setWorkingDirectory(dir.absolutePath());
        zsync_windows_downloader->start("zsync2.exe", arguments);
        connect(zsync_windows_downloader, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &GarudaDownloader::onDownloadStop);
#endif

        this->ui->downloadButton->setText("Cancel");
        this->ui->statusText->setText("Initializing");
        setButtonStates(true);

        finished = false;
        zsync_updatetimer.start(500);
    }
    else
    {
#if __UNIX__
        zsync_downloader->terminate();
#else
        zsync_windows_downloader->kill();
#endif
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
#if __UNIX__
        if (zsync_client->nextStatusMessage(out))
        {
            while (zsync_client->nextStatusMessage(out));
            this->ui->statusText->setText(QString::fromStdString(out));
        }
#endif
    }

    QApplication::alert(this);
}

void GarudaDownloader::onEtcherDownloadFinished(bool success)
{
#if __LINUX__
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
#endif
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

void GarudaDownloader::onDownloadStop()
{
    this->ui->downloadButton->setText("Download");
    if (!finished)
    {
        this->ui->progressBar->setValue(0);
        this->ui->statusText->setText("Idle");
    }

    setButtonStates(false);

#if __UNIX__
    delete zsync_client;
    zsync_client = nullptr;
    zsync_downloader->deleteLater();
#else
    // We need to trigger this again before we delete zsync_windows_downloader
    // Otherwise we will never know what the result of the process is
    onUpdate();
    // We also need to stop the timer here, otherwise we will access a nullptr (not fun)
    zsync_updatetimer.stop();
    delete zsync_windows_downloader;
    zsync_windows_downloader = nullptr;
#endif

    // Clean up some files...
    QDir wildcards(dir.absolutePath());
    wildcards.setNameFilters({"*.zs-old", "rcksum-*", "rufus.exe"});
    for(const QString & filename: wildcards.entryList()) {
        QFile file(dir.absoluteFilePath(filename));
        file.setPermissions(file.permissions() |
                                QFileDevice::WriteOwner |
                                QFileDevice::WriteUser |
                                QFileDevice::WriteGroup |
                                QFileDevice::WriteOther);
        file.remove();
    }
}

void GarudaDownloader::setButtonStates(bool downloading)
{
    this->ui->selectButton->setDisabled(downloading);
    this->ui->flashButton->setDisabled(downloading ? true : (!QFile::exists(dir.absoluteFilePath("current.iso"))));
#if !__UNIX__
    ui->selectButton->hide();
#endif
}

void GarudaDownloader::onUpdate()
{
#if __UNIX__
    this->ui->progressBar->setValue(zsync_client->progress() * 100);
    std::string out;
    while (zsync_client->nextStatusMessage(out))
    {
        qInfo() << QString::fromStdString(out);
        if (out.rfind("optimized ranges,", 0) == 0)
            continue;
        this->ui->statusText->setText(QString::fromStdString(out));
    }
#else
    auto output = QString(zsync_windows_downloader->readAllStandardOutput()).split("\n");
    for (auto &entry : output)
    {
        if (entry.isEmpty())
            continue;
        qInfo() << entry;
        if (entry.toStdString().rfind("optimized ranges,", 0) == 0)
            continue;
        if (entry == "checksum matches OK")
        {
            onDownloadFinished(true);
            return;
        }
        if (entry.startsWith("\r"))
        {
            QRegExp expression("\\d+(?:\\.\\d+)?%");
            expression.indexIn(entry);
            auto captured = expression.capturedTexts();
            qInfo() << captured;
            bool ok = false;
            int number = captured.first().split(".").first().toInt(&ok);
            if (ok)
                this->ui->progressBar->setValue(number);
        }
        else
                this->ui->statusText->setText(entry);
    }
    auto error = QString(zsync_windows_downloader->readAllStandardError()).split("\n");
    for (auto &entry : error)
    {
        if (entry.isEmpty())
            continue;
        qInfo() << entry;
        if (entry.toStdString().rfind("optimized ranges,", 0) == 0)
            continue;
        this->ui->statusText->setText(entry);
    }
#endif
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

/*void GarudaDownloader::on_flashButton_clicked()
{
#if __UNIX__
    if (!zsync_client)
#else
    if (true)
#endif
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
#if __UNIX__
            zsync_client = new zsync2::ZSyncClient(path.toStdString(), "etcher.AppImage");
            zsync_client->setCwd(dir.absolutePath().toStdString());
            zsync_client->setRangesOptimizationThreshold(64 * 4096);
            zsync_downloader = new ZSyncDownloader(zsync_client);
            connect(zsync_downloader, &ZSyncDownloader::done, this, &GarudaDownloader::onEtcherDownloadFinished);
            connect(zsync_downloader, &ZSyncDownloader::finished, this, &GarudaDownloader::onDownloadStop);
            zsync_downloader->start();
#endif

            this->ui->downloadButton->setText("Cancel");
            this->ui->statusText->setText("Initializing");
            setButtonStates(true);

            finished = false;
            zsync_updatetimer.start(500);
        }
    }
}*/

void GarudaDownloader::on_flashButton_clicked()
{
#if __UNIX__
    if (!zsync_client)
#else
    if (!zsync_windows_downloader)
#endif
    {
        if (!dir.exists())
            if (!QDir().mkdir(dir.absolutePath()))
                return;

#if __LINUX__
        if (QFile::exists("/usr/bin/balena-etcher-electron"))
        {
            this->hide();
            QProcess::execute("/usr/bin/balena-etcher-electron", { dir.absoluteFilePath("./current.iso") });
            this->show();
            return;
        }

        auto path = dir.absoluteFilePath("etcher.zsync");
        bool out = QFile::copy(":/main/resources/etcher.zsync", path);
        zsync_client = new zsync2::ZSyncClient(path.toStdString(), "etcher.AppImage");
        zsync_client->setCwd(dir.absolutePath().toStdString());
        zsync_client->setRangesOptimizationThreshold(64 * 4096);
        zsync_downloader = new ZSyncDownloader(zsync_client);
        connect(zsync_downloader, &ZSyncDownloader::done, this, &GarudaDownloader::onEtcherDownloadFinished);
        connect(zsync_downloader, &ZSyncDownloader::finished, this, &GarudaDownloader::onDownloadStop);
        zsync_downloader->start();
        this->ui->downloadButton->setText("Cancel");
        this->ui->statusText->setText("Initializing");
        setButtonStates(true);

        finished = false;
        zsync_updatetimer.start(500);
#else
    auto path = dir.absoluteFilePath("rufus.exe");
    QFile::copy(":/windows/resources/rufus.exe", path);
    this->hide();

    SHELLEXECUTEINFO ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = "runas";
    ShExecInfo.lpFile = path.toUtf8().constData();
    ShExecInfo.lpParameters = "-g -i current.iso";
    ShExecInfo.lpDirectory = dir.absolutePath().toUtf8().constData();
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    ShellExecuteEx(&ShExecInfo);
    WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
    CloseHandle(ShExecInfo.hProcess);
    this->show();
#endif
    }
}

void GarudaDownloader::on_statusText_linkActivated(const QString &link)
{
    if (link == "#open_folder")
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}
