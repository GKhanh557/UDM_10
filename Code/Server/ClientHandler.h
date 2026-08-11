#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QString>
#include <QByteArray>

// Moi khi co Client ket noi de upload 1 file, Server se tao 1 ClientHandler
// rieng cho ket noi do. Nho vay loi o 1 file/1 ket noi khong anh huong
// den cac file/ket noi khac dang chay dong thoi (dap ung yeu cau de bai).
class ClientHandler : public QObject
{
    Q_OBJECT

public:
    explicit ClientHandler(QTcpSocket *socket, const QString &saveDir, QObject *parent = nullptr);
    ~ClientHandler() override;

    QString clientAddress() const;

signals:
    // Phat ra moi khi co su kien can ghi log / hien thi len GUI Server
    // status vi du: "Dang nhan", "Hoan tat", "Loi"
    void logEvent(const QString &clientAddr, const QString &fileName,
                  const QString &status, const QString &detail);

    // Phat ra khi handler da xu ly xong (thanh cong hoac loi), co the xoa an toan
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

    QString m_fileName;      // ten file goc (da giai ma)
    QString m_finalPath;     // duonng dan cuoi cung se luu (da xu ly trung ten)
    QString m_tempPath;      // duonng dan file tam trong luc dang nhan
    qint64  m_fileSize = 0;
    qint64  m_bytesReceived = 0;

    QFile m_outFile;
};
