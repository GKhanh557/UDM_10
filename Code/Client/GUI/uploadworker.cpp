#include "uploadworker.h"

#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QUrl>

namespace {
constexpr qint64 kChunkSize = 64 * 1024;      // đọc/gửi 64KB mỗi lần
constexpr int kProgressIntervalMs = 150;      // throttle tín hiệu progress
}

UploadWorker::UploadWorker(const QString &filePath, const QString &host, quint16 port)
    : m_filePath(filePath), m_host(host), m_port(port)
{
    setAutoDelete(true); // QThreadPool tự xóa worker sau khi run() xong
}

void UploadWorker::cancel()
{
    m_cancelled = true;
}

QString UploadWorker::percentEncodeFileName(const QString &name)
{
    // percent-encode giống encode URL, tránh lỗi khoảng trắng / tiếng Việt / ký tự đặc biệt
    return QUrl::toPercentEncoding(name);
}

void UploadWorker::run()
{
    emit started(m_filePath);
    emit statusChanged(m_filePath, QStringLiteral("Đang tải"));

    QFile file(m_filePath);
    QFileInfo info(m_filePath);
    const QString fileName = info.fileName();
    const qint64 fileSize = info.size();

    if (!file.open(QIODevice::ReadOnly)) {
        emit statusChanged(m_filePath, QStringLiteral("Lỗi: không mở được file nguồn"));
        emit finished(m_filePath, false);
        return;
    }

    QTcpSocket socket;
    socket.connectToHost(m_host, m_port);
    if (!socket.waitForConnected(5000)) {
        emit statusChanged(m_filePath, QStringLiteral("Lỗi: không kết nối được Server (%1)").arg(socket.errorString()));
        emit finished(m_filePath, false);
        return;
    }

    // ---- Gửi header: ULD1 <ten_encoded> <kich_thuoc>\n ----
    const QByteArray header = QStringLiteral("ULD1 %1 %2\n")
                                   .arg(percentEncodeFileName(fileName))
                                   .arg(fileSize)
                                   .toUtf8();

    if (socket.write(header) != header.size() || !socket.waitForBytesWritten(5000)) {
        emit statusChanged(m_filePath, QStringLiteral("Lỗi: gửi header thất bại"));
        emit finished(m_filePath, false);
        return;
    }

    // ---- Gửi dữ liệu file theo từng chunk, tự cập nhật % và tốc độ ----
    qint64 bytesSent = 0;
    QElapsedTimer speedTimer;
    QElapsedTimer progressTimer;
    speedTimer.start();
    progressTimer.start();
    qint64 bytesSinceLastTick = 0;

    QByteArray buffer;
    buffer.resize(kChunkSize);

    while (!file.atEnd()) {
        if (m_cancelled.load()) {
            socket.abort();
            emit statusChanged(m_filePath, QStringLiteral("Đã hủy"));
            emit finished(m_filePath, false);
            return;
        }

        const qint64 readBytes = file.read(buffer.data(), kChunkSize);
        if (readBytes < 0) {
            emit statusChanged(m_filePath, QStringLiteral("Lỗi: đọc file nguồn thất bại"));
            emit finished(m_filePath, false);
            return;
        }
        if (readBytes == 0) break;

        qint64 written = 0;
        while (written < readBytes) {
            const qint64 n = socket.write(buffer.constData() + written, readBytes - written);
            if (n < 0 || !socket.waitForBytesWritten(10000)) {
                emit statusChanged(m_filePath, QStringLiteral("Lỗi: mất kết nối khi đang gửi (%1)").arg(socket.errorString()));
                emit finished(m_filePath, false);
                return;
            }
            written += n;
        }

        bytesSent += readBytes;
        bytesSinceLastTick += readBytes;

        // Throttle: chỉ emit progress mỗi ~150ms để không làm nghẽn GUI thread
        if (progressTimer.elapsed() >= kProgressIntervalMs || bytesSent == fileSize) {
            const double elapsedSec = speedTimer.elapsed() / 1000.0;
            const double speed = elapsedSec > 0 ? bytesSinceLastTick / elapsedSec : 0.0;
            emit progressChanged(m_filePath, bytesSent, fileSize, speed);
            progressTimer.restart();
            speedTimer.restart();
            bytesSinceLastTick = 0;
        }
    }

    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.waitForDisconnected(3000);
    }

    emit statusChanged(m_filePath, QStringLiteral("Hoàn tất"));
    emit finished(m_filePath, true);
}
