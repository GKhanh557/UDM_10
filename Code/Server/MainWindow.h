#pragma once
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QMap>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QListWidget;
class QCloseEvent;
QT_END_NAMESPACE

// Neu Client ket noi nhung khong gui header trong khoang thoi gian nay thi Server tu ngat,
// tranh giu tai nguyen (socket) vo han cho client "treo"
const int HEADER_TIMEOUT_MS = 10000;

// Struct nhỏ lưu thông tin đang nhận file của 1 client
struct ReceivingFile {
    bool headerDone = false;
    QString fileName;
    qint64 fileSize = 0;
    qint64 received = 0;
    QFile *file = nullptr;
    QString tempPath;  // duong dan file tam (.part) trong luc dang nhan
    QString finalPath; // duong dan file that su sau khi nhan xong
};

class ServerWindow : public QMainWindow
{
    Q_OBJECT
public:
    ServerWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void startServer();
    void newClient();
    void clientReadyRead();
    void clientDisconnected();

private:
    QTcpServer *server;
    QLineEdit *portEdit;
    QLineEdit *folderEdit;
    QPushButton *startBtn;
    QListWidget *logList;

    // mỗi socket đang kết nối sẽ có 1 ReceivingFile tương ứng
    QMap<QTcpSocket*, ReceivingFile> clients;

    QString makeUniqueName(const QString &name);
    void addLog(const QString &message); // ghi log kèm thời gian, theo yeu cau bat buoc
};
