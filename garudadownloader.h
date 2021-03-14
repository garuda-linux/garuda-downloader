#ifndef GARUDADOWNLOADER_H
#define GARUDADOWNLOADER_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class GarudaDownloader; }
QT_END_NAMESPACE

namespace zsync2 { class ZSyncClient; }

class ZSyncDownloader : public QThread
{
    Q_OBJECT
public:
    ZSyncDownloader(zsync2::ZSyncClient *client) : client(client) {}
private:
    void run() override;
    zsync2::ZSyncClient *client;
signals:
    void done(bool success);
};

class GarudaDownloader : public QMainWindow
{
    Q_OBJECT

public:
    GarudaDownloader(QWidget *parent = nullptr);
    ~GarudaDownloader();

private slots:
    void on_downloadButton_clicked();
    void onUpdate();
    void on_selectButton_clicked();
    void on_flashButton_clicked();

private:
    void onDownloadFinished(bool success);
    void onDownlaodStop();
    void setButtonStates(bool downloading);
    void onEtcherDownloadFinished(bool success);

    Ui::GarudaDownloader *ui;
    zsync2::ZSyncClient *zsync_client = nullptr;
    ZSyncDownloader *zsync_downloader = nullptr;;
    QTimer zsync_updatetimer;
    QString seed_file;
    bool finished;
};
#endif // GARUDADOWNLOADER_H
