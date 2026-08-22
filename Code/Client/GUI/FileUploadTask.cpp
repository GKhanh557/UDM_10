#include "FileUploadTask.h"

#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

FileUploadTask::FileUploadTask(const QString &filePath, const QString &host, quint16 port, QObject *parent)
    : QObject(parent), m_filePath(filePath), m_host(host), m_port(port)
{
}

FileUploadTask::~FileUploadTask()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }
}

QByteArray FileUploadTask::percentEncodeFileName(const QString &name)
{
    return QUrl::toPercentEncoding(name);
}

void FileUploadTask::start()
{
    QFileInfo info(m_filePath);
    m_fileSize = info.size();

    m_file = new QFile(m_filePath, this);
    if (!m_file->open(QIODevice::ReadOnly)) {
        finishWithError(QStringLiteral("không mở được file nguồn"));
        return;
    }

    emit statusChanged(QStringLiteral("Đang tải"));
    m_speedTimer.start();

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &FileUploadTask::onConnected);
    connect(m_socket, &QTcpSocket::bytesWritten, this, &FileUploadTask::onBytesWritten);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &FileUploadTask::onSocketError);

    m_socket->connectToHost(m_host, m_port);
}

void FileUploadTask::cancel()
{
    if (m_done) return;
    m_cancelled = true;
    if (m_socket) m_socket->abort();
    emit statusChanged(QStringLiteral("Đã hủy"));
    emit finished(false);
    m_done = true;
}

void FileUploadTask::onConnected()
{
    const QFileInfo info(m_filePath);
    const QByteArray header = "ULD1 " + percentEncodeFileName(info.fileName())
                             + ' ' + QByteArray::number(m_fileSize) + '\n';
    m_socket->write(header);
}

void FileUploadTask::onBytesWritten(qint64)
{
    if (m_cancelled || m_done) return;

    if (m_socket->bytesToWrite() == 0) {
        sendNextChunk();
    }
}

void FileUploadTask::sendNextChunk()
{
    if (m_file->atEnd()) {
        finishSuccess();
        return;
    }

    const QByteArray chunk = m_file->read(kChunkSize);
    if (chunk.isEmpty() && !m_file->atEnd()) {
        finishWithError(QStringLiteral("đọc file nguồn thất bại"));
        return;
    }

    m_socket->write(chunk);
    m_bytesSent += chunk.size();
    m_bytesSinceLastTick += chunk.size();

    // Throttle progress signal ~150ms/lần để không làm nghẽn GUI.
    const qint64 elapsedMs = m_speedTimer.elapsed();
    if (elapsedMs - m_lastProgressEmitMs >= 150 || m_bytesSent == m_fileSize) {
        const double elapsedSec = elapsedMs / 1000.0;
        const double speed = elapsedSec > 0 ? m_bytesSinceLastTick / elapsedSec : 0.0;
        emit progressChanged(m_bytesSent, m_fileSize, speed);
        m_lastProgressEmitMs = elapsedMs;
        m_bytesSinceLastTick = 0;
        m_speedTimer.restart();
    }

    if (m_file->atEnd()) {
        if (m_socket->bytesToWrite() == 0) {
            finishSuccess();
        }
    }
}

void FileUploadTask::finishSuccess()
{
    if (m_done) return;
    m_done = true;
    m_socket->disconnectFromHost();
    emit statusChanged(QStringLiteral("Hoàn tất"));
    emit finished(true);
}

void FileUploadTask::finishWithError(const QString &reason)
{
    if (m_done) return;
    m_done = true;
    if (m_socket) m_socket->abort();
    emit statusChanged(QStringLiteral("Lỗi: %1").arg(reason));
    emit finished(false);
}

void FileUploadTask::onSocketError()
{
    if (m_done) return;
    finishWithError(m_socket->errorString());
}
