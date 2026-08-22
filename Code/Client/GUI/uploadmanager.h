#pragma once

#include <QObject>
#include <QThreadPool>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QPointer>

class UploadWorker;

// UploadManager là lớp trung tâm nối GUI với các UploadWorker.
//
// Cách xử lý hàng đợi + giới hạn upload đồng thời:
//   QThreadPool.setMaxThreadCount(N) quyết định tối đa N worker chạy song
//   song; các file thêm vào sau khi đã đủ N sẽ tự động nằm trong hàng đợi
//   nội bộ của QThreadPool và được chạy dần khi có "slot" trống — không cần
//   tự cài đặt hàng đợi thủ công.
class UploadManager : public QObject
{
    Q_OBJECT

public:
    explicit UploadManager(QObject *parent = nullptr);

    void setServerAddress(const QString &host, quint16 port);

    // Số nhóm tối đa cho phép upload cùng lúc (theo đề bài, nhóm tự công bố).
    void setMaxConcurrentUploads(int maxConcurrent);
    int maxConcurrentUploads() const;

    // Thêm 1 hoặc nhiều file vào hàng đợi upload.
    void addFiles(const QStringList &filePaths);

    // Hủy 1 file đang tải hoặc đang chờ trong hàng đợi.
    void cancelFile(const QString &filePath);

signals:
    void fileQueued(const QString &filePath, qint64 fileSize);
    void fileStarted(const QString &filePath);
    void fileProgress(const QString &filePath, qint64 bytesSent, qint64 totalBytes, double speedBytesPerSec);
    void fileStatus(const QString &filePath, const QString &status);
    void fileFinished(const QString &filePath, bool success);

private:
    QThreadPool m_pool;
    QString m_host = QStringLiteral("127.0.0.1");
    quint16 m_port = 5000;

    // Theo dõi worker đang chạy để có thể cancel() từ GUI.
    QMap<QString, QPointer<UploadWorker>> m_activeWorkers;
};
