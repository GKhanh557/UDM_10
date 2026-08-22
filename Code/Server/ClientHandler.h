#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QString>
#include <QByteArray>

class ClientHandler : public QObject
{
    Q_OBJECT

public:
    explicit ClientHandler(QTcpSocket *socket, const QString &saveDir, QObject *parent = nullptr);
    ~ClientHandler() override;

    QString clientAddress() const;

signals:
    void logEvent(const QString &clientAddr, const QString &fileName,
                  const QString &status, const QString &detail);

    void finishedHandling();

private slots:
    void onReadyRead();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError err);

private:
    enum class State { ReadingHeader, ReadingData, Done, Failed };

    bool tryParseHeaderLine(const QByteArray &line);
    QString resolveDuplicateName(const QString &dir, const QString &fileName) const;
    void failWithError(const QString &reason);
    void finishSuccess();

    QTcpSocket *m_socket = nullptr;
    QString m_saveDir;
    QString m_clientAddr;

    State m_state = State::ReadingHeader;

    QString m_fileName;      
    QString m_finalPath;     
    QString m_tempPath;     
    qint64  m_fileSize = 0;
    qint64  m_bytesReceived = 0;

    QFile m_outFile;
};
