#ifndef GARUDADOWNLOADER_H
#define GARUDADOWNLOADER_H

#include <QMainWindow>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>

#if __unix__
#include <QThread>
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class GarudaDownloader; }
QT_END_NAMESPACE

class QProcess;

#if __unix__
namespace zsync2 { class ZSyncClient; }
#endif

#if __unix__
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
#endif

class GarudaDownloader : public QMainWindow
{
    Q_OBJECT

public:
    GarudaDownloader(QWidget *parent = nullptr);
    ~GarudaDownloader();

private slots:
    void on_downloadButton_clicked();
    void onUpdate();
#if __unix__
    void on_selectButton_clicked();
#endif
    void on_flashButton_clicked();
    void on_statusText_linkActivated(const QString &link);
    void onEditionlistDownloaded();

private:
    void onDownloadFinished(bool success);
    void onDownloadStop();
    void setButtonStates(bool downloading);
    void resizeEvent(QResizeEvent* event) override;

#if __unix__
    void onEtcherDownloadFinished(bool success);
    void updateSelectSize();
#endif

    Ui::GarudaDownloader *ui;
#if __unix__
    zsync2::ZSyncClient *zsync_client = nullptr;
    ZSyncDownloader *zsync_downloader = nullptr;
#else
    QProcess *zsync_windows_downloader = nullptr;
#endif
    QTimer zsync_updatetimer;
    QString seed_file;

    QNetworkAccessManager network_manager;
    QNetworkReply *network_reply;

    bool finished = false;
};
#endif // GARUDADOWNLOADER_H
