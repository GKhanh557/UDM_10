#pragma once

#include <QObject>
#include <QRunnable>
#include <QString>
#include <atomic>

// Mỗi UploadWorker chịu trách nhiệm upload DUY NHẤT 1 file, dùng 1 socket
// TCP riêng, đúng theo giao thức trong README:
//   ULD1 <ten_file_percent_encoded> <kich_thuoc>\n
//   <fileSize byte du lieu nhi phan>
//
// Worker chạy trong QThreadPool -> lỗi/exception của 1 file KHÔNG ảnh hưởng
// tới các worker khác đang chạy song song.
class UploadWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    UploadWorker(const QString &filePath, const QString &host, quint16 port);

    void run() override;

    // Gọi từ thread GUI để hủy upload đang chạy (nếu có).
    void cancel();

signals:
    // Phát định kỳ trong lúc gửi dữ liệu.
    void progressChanged(const QString &filePath, qint64 bytesSent, qint64 totalBytes, double speedBytesPerSec);

    // Phát khi trạng thái đổi: "Đang chờ", "Đang tải", "Hoàn tất", "Lỗi: <lý do>", "Đã hủy"
    void statusChanged(const QString &filePath, const QString &status);

    void started(const QString &filePath);
    void finished(const QString &filePath, bool success);

private:
    QString m_filePath;
    QString m_host;
    quint16 m_port;
    std::atomic_bool m_cancelled{false};

    static QString percentEncodeFileName(const QString &name);
};
