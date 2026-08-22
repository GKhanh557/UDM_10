#pragma once

#include <QObject>
#include <QString>
#include <QElapsedTimer>

class QTcpSocket;
class QFile;

class FileUploadTask : public QObject
{
    Q_OBJECT

public:
    FileUploadTask(const QString &filePath, const QString &host, quint16 port, QObject *parent = nullptr);
    ~FileUploadTask() override;

    void start();

    void cancel();

    QString filePath() const { return m_filePath; }

signals:
    void progressChanged(qint64 bytesSent, qint64 totalBytes, double speedBytesPerSec);

    void statusChanged(const QString &status);

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
