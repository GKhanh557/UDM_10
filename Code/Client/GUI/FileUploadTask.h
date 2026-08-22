#pragma once

#include <QObject>
#include <QString>
#include <QElapsedTimer>

class QTcpSocket;
class QFile;

// FileUploadTask chịu trách nhiệm upload DUY NHẤT 1 file qua 1 QTcpSocket
// riêng, đúng giao thức mô tả trong README:
//   ULD1 <ten_file_percent_encoded> <kich_thuoc>\n
//   <fileSize byte du lieu nhi phan>
//
// Task chạy HOÀN TOÀN BẤT ĐỒNG BỘ trên chính event loop của GUI thread
// (không dùng thread/blocking call) — nhiều FileUploadTask có thể chạy
// song song vì QTcpSocket là non-blocking, việc gửi từng chunk được điều
// khiển qua tín hiệu bytesWritten(). Vì vậy 1 file lỗi (mất kết nối, socket
// lỗi...) chỉ phát statusChanged/finished(false) của riêng nó, không đụng
// tới các FileUploadTask khác đang chạy.
class FileUploadTask : public QObject
{
    Q_OBJECT

public:
    FileUploadTask(const QString &filePath, const QString &host, quint16 port, QObject *parent = nullptr);
    ~FileUploadTask() override;

    void start();

    // Gọi để hủy giữa chừng (vd khi user đóng app hoặc bấm hủy dòng đó).
    void cancel();

    QString filePath() const { return m_filePath; }

signals:
    // Phát định kỳ (throttled) trong lúc gửi dữ liệu.
    void progressChanged(qint64 bytesSent, qint64 totalBytes, double speedBytesPerSec);

    // "Đang tải", "Hoàn tất", "Lỗi: <lý do>", "Đã hủy"
    void statusChanged(const QString &status);

    // success = true nếu gửi xong toàn bộ file và đóng kết nối bình thường.
    void finished(bool success);

private slots:
    void onConnected();
    void onBytesWritten(qint64 bytes);
    void onSocketError();

private:
    void sendNextChunk();
    void finishWithError(const QString &reason);
    void finishSuccess();
    static QByteArray percentEncodeFileName(const QString &name);

    QString m_filePath;
    QString m_host;
    quint16 m_port;

    QTcpSocket *m_socket = nullptr;
    QFile *m_file = nullptr;

    qint64 m_fileSize = 0;
    qint64 m_bytesSent = 0;
    bool m_cancelled = false;
    bool m_done = false;

    QElapsedTimer m_speedTimer;
    qint64 m_bytesSinceLastTick = 0;
    qint64 m_lastProgressEmitMs = 0;

    static constexpr qint64 kChunkSize = 64 * 1024;
};
