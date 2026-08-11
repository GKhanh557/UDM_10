#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QString>
#include <QByteArray>

// ClientHandler: quản lý một kết nối TCP dùng để nhận đúng MỘT file.
// Mỗi khi có Client kết nối để upload 1 file, Server sẽ tạo 1 ClientHandler
// riêng cho kết nối đó. Nhờ vậy lỗi ở 1 file/1 kết nối không ảnh hưởng
// đến các file/kết nối khác đang chạy đồng thời (đáp ứng yêu cầu đề bài).
class ClientHandler : public QObject
{
    Q_OBJECT

public:
    // socket: lấy trực tiếp từ QTcpServer::nextPendingConnection(), ClientHandler
    //         sẽ nhận sở hữu (reparent) socket này.
    // saveDir: thư mục Server sẽ lưu file nhận được
    explicit ClientHandler(QTcpSocket *socket, const QString &saveDir, QObject *parent = nullptr);
    ~ClientHandler() override;

    QString clientAddress() const;

signals:
    // Phát ra mỗi khi có sự kiện cần ghi log / hiển thị lên GUI Server
    // status ví dụ: "Đang nhận", "Hoàn tất", "Lỗi"
    void logEvent(const QString &clientAddr, const QString &fileName,
                  const QString &status, const QString &detail);

    // Phát ra khi handler đã xử lý xong (thành công hoặc lỗi), có thể xoá an toàn
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

    QString m_fileName;      // tên file gốc (đã giải mã)
    QString m_finalPath;     // đường dẫn cuối cùng sẽ lưu (đã xử lý trùng tên)
    QString m_tempPath;      // đường dẫn file tạm trong lúc đang nhận
    qint64  m_fileSize = 0;
    qint64  m_bytesReceived = 0;

    QFile m_outFile;
};
