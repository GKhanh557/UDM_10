#pragma once

#include <QObject>
#include <QThreadPool>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QPointer>

class UploadWorker;

class UploadManager : public QObject
{
    Q_OBJECT

public:
    explicit UploadManager(QObject *parent = nullptr);

    void setServerAddress(const QString &host, quint16 port);

    void setMaxConcurrentUploads(int maxConcurrent);
    int maxConcurrentUploads() const;

    void addFiles(const QStringList &filePaths);

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

    QMap<QString, QPointer<UploadWorker>> m_activeWorkers;
};
