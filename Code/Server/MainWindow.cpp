#include "MainWindow.h"
#include "../Shared/ProtocolCommon.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("UDM_10 - Server");
    resize(600, 400);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    QHBoxLayout *row1 = new QHBoxLayout();
    portEdit = new QLineEdit("5000", this);
    startBtn = new QPushButton("Start Server", this);
    row1->addWidget(new QLabel("Port:"));
    row1->addWidget(portEdit);
    row1->addWidget(startBtn);
    mainLayout->addLayout(row1);

    QHBoxLayout *row2 = new QHBoxLayout();
    folderEdit = new QLineEdit(QDir::currentPath() + "/received", this);
    QPushButton *browseBtn = new QPushButton("Chon thu muc", this);
    row2->addWidget(new QLabel("Luu vao:"));
    row2->addWidget(folderEdit);
    row2->addWidget(browseBtn);
    mainLayout->addLayout(row2);

    logList = new QListWidget(this);
    mainLayout->addWidget(logList);

    setCentralWidget(central);

    server = new QTcpServer(this);

    connect(startBtn, &QPushButton::clicked, this, &MainWindow::startServer);
    connect(server, &QTcpServer::newConnection, this, &MainWindow::newClient);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Chon thu muc");
        if (!dir.isEmpty())
            folderEdit->setText(dir);
    });
}

void MainWindow::startServer()
{
    quint16 port = portEdit->text().toUShort();
    QDir().mkpath(folderEdit->text());

    if (server->listen(QHostAddress::Any, port)) {
        logList->addItem("Server dang chay tren port " + QString::number(port));
        startBtn->setEnabled(false);
        portEdit->setEnabled(false);
    } else {
        QMessageBox::warning(this, "Loi", "Khong mo duoc port: " + server->errorString());
    }
}

void MainWindow::newClient()
{
    QTcpSocket *socket = server->nextPendingConnection();
    clients[socket] = ReceivingFile(); // tạo state mới cho client này

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::clientReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::clientDisconnected);

    logList->addItem("Client moi ket noi: " + socket->peerAddress().toString());
}

void MainWindow::clientReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !clients.contains(socket))
        return;

    ReceivingFile &info = clients[socket];

    // Chưa có header thì đọc 1 dòng để lấy tên file + kích thước
    if (!info.headerDone) {
        if (!socket->canReadLine())
            return; // đợi đủ dữ liệu rồi mới đọc

        QByteArray line = socket->readLine().trimmed();
        QList<QByteArray> parts = line.split('|');
        if (parts.size() != 3 || parts[0] != Protocol::MAGIC) {
            logList->addItem("Header sai dinh dang, bo qua client nay");
            socket->disconnectFromHost();
            return;
        }

        info.fileName = QFileInfo(QString::fromUtf8(parts[1])).fileName(); // bỏ đường dẫn nếu có
        info.fileSize = parts[2].toLongLong();
        info.received = 0;

        QString savePath = folderEdit->text() + "/" + makeUniqueName(info.fileName);
        info.file = new QFile(savePath);
        info.file->open(QIODevice::WriteOnly);

        info.headerDone = true;
        logList->addItem("Dang nhan: " + info.fileName + " (" + QString::number(info.fileSize) + " bytes)");
    }

    // Đọc phần dữ liệu file còn lại
    QByteArray data = socket->readAll();
    if (!data.isEmpty() && info.file) {
        info.file->write(data);
        info.received += data.size();
    }

    if (info.received >= info.fileSize && info.file) {
        info.file->close();
        logList->addItem("Hoan tat: " + info.fileName);
        socket->disconnectFromHost();
    }
}

void MainWindow::clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    if (clients.contains(socket)) {
        ReceivingFile &info = clients[socket];
        // Nếu chưa nhận đủ dữ liệu mà mất kết nối -> báo lỗi, xoá file dở
        if (info.file) {
            if (info.received < info.fileSize) {
                logList->addItem("Loi: mat ket noi giua chung khi nhan " + info.fileName);
                info.file->remove();
            }
            delete info.file;
        }
        clients.remove(socket);
    }
    socket->deleteLater();
}

QString MainWindow::makeUniqueName(const QString &name)
{
    // Neu file da ton tai thi them _1, _2... vao truoc phan mo rong
    QString path = folderEdit->text() + "/" + name;
    if (!QFile::exists(path))
        return name;

    QFileInfo fi(name);
    QString base = fi.completeBaseName();
    QString ext = fi.suffix();

    int i = 1;
    QString newName;
    do {
        newName = ext.isEmpty() ? QString("%1_%2").arg(base).arg(i)
                                 : QString("%1_%2.%3").arg(base).arg(i).arg(ext);
        i++;
    } while (QFile::exists(folderEdit->text() + "/" + newName));

    return newName;
}
