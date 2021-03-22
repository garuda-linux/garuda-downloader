#ifndef GARUDADOWNLOADER_H
#define GARUDADOWNLOADER_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>

// Zsync
#if __WIN32__
#include <QProcess>
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class GarudaDownloader; }
QT_END_NAMESPACE

#if __UNIX__
namespace zsync2 { class ZSyncClient; }
#endif

#if __UNIX__
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
    void on_selectButton_clicked();
    void on_flashButton_clicked();

    void on_statusText_linkActivated(const QString &link);

private:
    void onDownloadFinished(bool success);
    void onDownloadStop();
    void setButtonStates(bool downloading);
    void onEtcherDownloadFinished(bool success);
    void updateSelectSize();
    void resizeEvent(QResizeEvent* event) override;

    Ui::GarudaDownloader *ui;
#if __UNIX__
    zsync2::ZSyncClient *zsync_client = nullptr;
    ZSyncDownloader *zsync_downloader = nullptr;
#else
    QProcess *zsync_windows_downloader = nullptr;
#endif
    QTimer zsync_updatetimer;
    QString seed_file;
    bool finished = false;
};
#endif // GARUDADOWNLOADER_H
