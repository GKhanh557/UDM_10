#pragma once
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QMap>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QListWidget;
QT_END_NAMESPACE

// Struct nhỏ lưu thông tin đang nhận file của 1 client
struct ReceivingFile {
    bool headerDone = false;
    QString fileName;
    qint64 fileSize = 0;
    qint64 received = 0;
    QFile *file = nullptr;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

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
};
