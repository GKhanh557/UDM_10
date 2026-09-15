#pragma once
#include <QMainWindow>
#include <QTcpSocket>
#include <QFile>
#include <QMap>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QTableWidget;
class QProgressBar;
QT_END_NAMESPACE

const int MAX_CONCURRENT_UPLOADS = 2;

struct UploadInfo {
    QString filePath;
    qint64 fileSize = 0;
    qint64 sent = 0;
    QFile *file = nullptr;
    int row = -1;

    qint64 lastTickMs = 0;
    qint64 sentSinceLastTick = 0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void chooseFiles();
    void uploadAll();
    void socketConnected();
    void socketBytesWritten(qint64 bytes);
    void socketError();

private:
    QLineEdit *hostEdit;
    QLineEdit *portEdit;
    QPushButton *chooseBtn;
    QPushButton *uploadBtn;
    QTableWidget *table;

    QStringList pendingFiles;
    QList<int> pendingRows; 
    QMap<QTcpSocket*, UploadInfo> uploads;
    int runningCount = 0;    

    void addFileRow(const QString &path);
    void tryStartNext(); 
    void startUpload(const QString &path, int row);
    void sendNextChunk(QTcpSocket *socket);
};
