#include "MainWindow.h"
#include "../Shared/ProtocolCommon.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("UDM_10 - Client");
    resize(700, 450);
    setAcceptDrops(true);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    QHBoxLayout *row1 = new QHBoxLayout();
    hostEdit = new QLineEdit("127.0.0.1", this);
    portEdit = new QLineEdit("5000", this);
    row1->addWidget(new QLabel("Server IP:"));
    row1->addWidget(hostEdit);
    row1->addWidget(new QLabel("Port:"));
    row1->addWidget(portEdit);
    mainLayout->addLayout(row1);

    QLabel *hint = new QLabel("Keo file vao day, hoac bam nut ben duoi", this);
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet("border: 2px dashed gray; padding: 15px;");
    mainLayout->addWidget(hint);

    QHBoxLayout *row2 = new QHBoxLayout();
    chooseBtn = new QPushButton("Chon file...", this);
    uploadBtn = new QPushButton("Upload tat ca", this);
    row2->addWidget(chooseBtn);
    row2->addWidget(uploadBtn);
    mainLayout->addLayout(row2);

    table = new QTableWidget(0, 4, this);
    table->setHorizontalHeaderLabels({"Ten file", "Trang thai", "Tien trinh", "Toc do"});
    mainLayout->addWidget(table);

    setCentralWidget(central);

    connect(chooseBtn, &QPushButton::clicked, this, &MainWindow::chooseFiles);
    connect(uploadBtn, &QPushButton::clicked, this, &MainWindow::uploadAll);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    for (const QUrl &url : event->mimeData()->urls()) {
        QString path = url.toLocalFile();
        if (QFileInfo(path).isFile())
            addFileRow(path);
    }
}

void MainWindow::chooseFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Chon file");
    for (const QString &f : files)
        addFileRow(f);
}

void MainWindow::addFileRow(const QString &path)
{
    int row = table->rowCount();
    table->insertRow(row);
    table->setItem(row, 0, new QTableWidgetItem(QFileInfo(path).fileName()));
    table->setItem(row, 1, new QTableWidgetItem("Cho"));

    QProgressBar *bar = new QProgressBar(this);
    bar->setValue(0);
    table->setCellWidget(row, 2, bar);

    table->setItem(row, 3, new QTableWidgetItem("-"));

    pendingFiles.append(path);
    pendingRows.append(row);
}

void MainWindow::uploadAll()
{
    // Chi cho phep toi da MAX_CONCURRENT_UPLOADS file chay cung luc,
    // file con lai nam trong hang doi (pendingFiles), trang thai van la "Cho"
    while (runningCount < MAX_CONCURRENT_UPLOADS && !pendingFiles.isEmpty()) {
        tryStartNext();
    }
}

void MainWindow::tryStartNext()
{
    if (pendingFiles.isEmpty())
        return;

    QString path = pendingFiles.takeFirst();
    int row = pendingRows.takeFirst();
    runningCount++;
    startUpload(path, row);
}

void MainWindow::startUpload(const QString &path, int row)
{
    QTcpSocket *socket = new QTcpSocket(this);

    UploadInfo info;
    info.filePath = path;
    info.fileSize = QFileInfo(path).size();
    info.file = new QFile(path);
    info.file->open(QIODevice::ReadOnly);
    info.row = row;

    uploads[socket] = info;

    connect(socket, &QTcpSocket::connected, this, &MainWindow::socketConnected);
    connect(socket, &QTcpSocket::bytesWritten, this, &MainWindow::socketBytesWritten);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &MainWindow::socketError);

    table->item(row, 1)->setText("Dang ket noi...");
    socket->connectToHost(hostEdit->text(), portEdit->text().toUShort());
}

void MainWindow::socketConnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !uploads.contains(socket))
        return;

    UploadInfo &info = uploads[socket];
    QString fileName = QFileInfo(info.filePath).fileName();

    // Gui header: ULD1|tenfile|kichthuoc
    QString header = QString("%1|%2|%3\n").arg("ULD1", fileName, QString::number(info.fileSize));
    socket->write(header.toUtf8());

    table->item(info.row, 1)->setText("Dang tai len...");
    sendNextChunk(socket);
}

void MainWindow::sendNextChunk(QTcpSocket *socket)
{
    UploadInfo &info = uploads[socket];

    QByteArray chunk = info.file->read(Protocol::CHUNK_SIZE);
    if (!chunk.isEmpty()) {
        socket->write(chunk);
        info.sent += chunk.size();
        info.sentSinceLastTick += chunk.size();

        int percent = info.fileSize > 0 ? (info.sent * 100 / info.fileSize) : 100;
        QProgressBar *bar = qobject_cast<QProgressBar*>(table->cellWidget(info.row, 2));
        if (bar) bar->setValue(percent);

        // Cu moi 500ms thi tinh lai toc do 1 lan, khong can tinh moi chunk cho do roi
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (info.lastTickMs == 0) info.lastTickMs = now;
        qint64 elapsed = now - info.lastTickMs;
        if (elapsed >= 500) {
            double kbps = (info.sentSinceLastTick / 1024.0) / (elapsed / 1000.0);
            table->item(info.row, 3)->setText(QString::number(kbps, 'f', 1) + " KB/s");
            info.lastTickMs = now;
            info.sentSinceLastTick = 0;
        }
    }
}

void MainWindow::socketBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !uploads.contains(socket))
        return;

    UploadInfo &info = uploads[socket];

    if (info.sent >= info.fileSize) {
        // Da gui xong, doi socket gui het buffer roi ngat ket noi
        if (socket->bytesToWrite() == 0) {
            table->item(info.row, 1)->setText("Hoan tat");
            table->item(info.row, 3)->setText("-");
            info.file->close();
            delete info.file;
            socket->disconnectFromHost();
            uploads.remove(socket);

            // Xong 1 file thi co 1 slot trong, lay file ke tiep trong hang doi ra chay
            runningCount--;
            tryStartNext();
        }
        return;
    }

    sendNextChunk(socket);
}

void MainWindow::socketError()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !uploads.contains(socket))
        return;

    UploadInfo &info = uploads[socket];
    // Loi 1 file khong lam dung cac file khac, chi bao loi dong nay
    table->item(info.row, 1)->setText("Loi: " + socket->errorString());
    table->item(info.row, 3)->setText("-");
    if (info.file) {
        info.file->close();
        delete info.file;
    }
    uploads.remove(socket);

    // 1 file loi cung tinh la het slot, lay file ke tiep trong hang doi ra chay
    runningCount--;
    tryStartNext();
}
