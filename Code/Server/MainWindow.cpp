#include "ServerWindow.h"
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
#include <QDateTime>
#include <QCloseEvent>

ServerWindow::ServerWindow(QWidget *parent) : QMainWindow(parent)
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

    connect(startBtn, &QPushButton::clicked, this, &ServerWindow::startServer);
    connect(server, &QTcpServer::newConnection, this, &ServerWindow::newClient);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Chon thu muc");
        if (!dir.isEmpty())
            folderEdit->setText(dir);
    });
}

void ServerWindow::startServer()
{
    quint16 port = portEdit->text().toUShort();
    QDir().mkpath(folderEdit->text());

    if (server->listen(QHostAddress::Any, port)) {
        addLog("Server dang chay tren port " + QString::number(port));
        startBtn->setEnabled(false);
        portEdit->setEnabled(false);
    } else {
        QMessageBox::warning(this, "Loi", "Khong mo duoc port: " + server->errorString());
    }
}

void ServerWindow::newClient()
{
    QTcpSocket *socket = server->nextPendingConnection();
    clients[socket] = ReceivingFile(); // tạo state mới cho client này

    connect(socket, &QTcpSocket::readyRead, this, &ServerWindow::clientReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ServerWindow::clientDisconnected);

    addLog("Client moi ket noi: " + socket->peerAddress().toString());

    // Neu qua HEADER_TIMEOUT_MS ma client van chua gui header thi ngat, tranh treo tai nguyen
    QTimer::singleShot(HEADER_TIMEOUT_MS, this, [this, socket]() {
        if (clients.contains(socket) && !clients[socket].headerDone) {
            addLog("Timeout: client khong gui du lieu, ngat ket noi");
            socket->disconnectFromHost();
        }
    });
}

void ServerWindow::clientReadyRead()
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

        // Kiem tra du lieu hop le truoc khi xu ly (yeu cau bat buoc: Server phai validate)
        bool valid = true;
        QString errorReason;

        if (parts.size() != 3 || parts[0] != Protocol::MAGIC) {
            valid = false;
            errorReason = "Sai dinh dang header";
        } else {
            bool sizeOk = false;
            qint64 size = parts[2].toLongLong(&sizeOk);
            if (!sizeOk || size < 0) {
                valid = false;
                errorReason = "Kich thuoc file khong hop le";
            } else if (QString::fromUtf8(parts[1]).trimmed().isEmpty()) {
                valid = false;
                errorReason = "Ten file rong";
            }
        }

        if (!valid) {
            addLog("Du lieu khong hop le tu client: " + errorReason);
            // Tra ve loi ro rang cho Client biet, thay vi im lang ngat ket noi
            socket->write(("ERROR|" + errorReason + "\n").toUtf8());
            socket->flush();
            socket->disconnectFromHost();
            return;
        }

        info.fileName = QFileInfo(QString::fromUtf8(parts[1])).fileName(); // bỏ đường dẫn nếu có
        info.fileSize = parts[2].toLongLong();
        info.received = 0;

        // Ghi ra file tam ".part" truoc. Chi doi ten thanh file that khi nhan DU du lieu,
        // tranh truong hop file dang ghi do bi hieu nham la du lieu hoan chinh.
        QString uniqueName = makeUniqueName(info.fileName);
        info.finalPath = folderEdit->text() + "/" + uniqueName;
        info.tempPath = info.finalPath + ".part";

        info.file = new QFile(info.tempPath);
        if (!info.file->open(QIODevice::WriteOnly)) {
           socket->write(QString("ERROR|Server khong the tao file de luu\n").toUtf8());
            socket->flush();
            delete info.file;
            info.file = nullptr;
            socket->disconnectFromHost();
            return;
        }

        info.headerDone = true;
        addLog("Dang nhan: " + info.fileName + " (" + QString::number(info.fileSize) + " bytes)");
    }

    // Đọc phần dữ liệu file còn lại
    QByteArray data = socket->readAll();
    if (!data.isEmpty() && info.file) {
        info.file->write(data);
        info.received += data.size();
    }

    if (info.received >= info.fileSize && info.file) {
        info.file->close();
        // Nhan du du lieu -> doi ten file tam sang ten that, luc nay moi cong nhan la hoan chinh
        QFile::rename(info.tempPath, info.finalPath);
        addLog("Hoan tat: " + info.fileName);
        socket->disconnectFromHost();
    }
}

void ServerWindow::clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    if (clients.contains(socket)) {
        ReceivingFile &info = clients[socket];
        // Nếu chưa nhận đủ dữ liệu mà mất kết nối -> báo lỗi, xoá file TAM dở dang
        // (không phải file thật, vì file thật chỉ tồn tại sau khi rename lúc hoàn tất)
        if (info.file) {
            if (info.received < info.fileSize) {
                addLog("Loi: mat ket noi giua chung khi nhan " + info.fileName);
                info.file->remove(); // xoa file .part dang do dang
            }
            delete info.file;
        }
        clients.remove(socket);
    }
    socket->deleteLater();
}

QString ServerWindow::makeUniqueName(const QString &name)
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

void ServerWindow::addLog(const QString &message)
{
    // Ghi log kem thoi gian - yeu cau bat buoc: log phai co thoi gian, ket noi,
    // ngat ket noi, loi va thao tac chinh
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss dd/MM/yyyy");
    logList->addItem("[" + time + "] " + message);
    logList->scrollToBottom();
}

void ServerWindow::closeEvent(QCloseEvent *event)
{
    // Neu dong cua so (vi du bam X) trong luc dang nhan file dang do, phai don dep
    // sach truoc khi thoat: ngat moi lien ket signal/slot va cac timer dang cho cua
    // tung socket, xoa file .part do dang. Neu khong lam buoc nay, mot QTimer::singleShot
    // dang cho (vi du timer cho header) co the "chay" dung luc doi tuong bat dau bi
    // huy trong qua trinh dong ung dung, gay ASSERT failure/crash.
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        QTcpSocket *socket = it.key();
        ReceivingFile &info = it.value();

        socket->disconnect(); // ngat het cac ket noi signal/slot lien quan socket nay
        socket->abort();

        if (info.file) {
            if (info.received < info.fileSize) {
                info.file->remove(); // xoa file .part do dang, dung nhu khi mat ket noi binh thuong
            }
            if (info.file->isOpen())
                info.file->close();
            delete info.file;
            info.file = nullptr;
        }
    }
    clients.clear();

    QMainWindow::closeEvent(event);
}
