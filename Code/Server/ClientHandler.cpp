#include "ClientHandler.h"
#include "../Shared/ProtocolCommon.h"

#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QDateTime>

ClientHandler::ClientHandler(QTcpSocket *socket, const QString &saveDir, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_saveDir(saveDir)
{
    m_socket->setParent(this); // ClientHandler nhan so huu socket
    m_clientAddr = QStringLiteral("%1:%2")
                        .arg(m_socket->peerAddress().toString())
                        .arg(m_socket->peerPort());

    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &ClientHandler::onSocketError);

    QDir().mkpath(m_saveDir);
}

ClientHandler::~ClientHandler()
{
    if (m_outFile.isOpen())
        m_outFile.close();
}

QString ClientHandler::clientAddress() const
{
    return m_clientAddr;
}

void ClientHandler::onReadyRead()
{
    // Buoc 1: doc dong header (ket thuc bang '\n')
    while (m_state == State::ReadingHeader && m_socket->canReadLine()) {
        const QByteArray line = m_socket->readLine();
        if (!tryParseHeaderLine(line)) {
            failWithError(QStringLiteral("Header khong hop le"));
            return;
        }
    }

    // Buoc 2: sau khi co header hop le, phan con lai la du lieu file tho
    if (m_state == State::ReadingData) {
        const QByteArray chunk = m_socket->readAll();
        if (!chunk.isEmpty()) {
            const qint64 written = m_outFile.write(chunk);
            if (written != chunk.size()) {
                failWithError(QStringLiteral("Loi ghi file tren Server"));
                return;
            }
            m_bytesReceived += written;
            m_outFile.flush();

            if (m_bytesReceived >= m_fileSize) {
                finishSuccess();
            }
        }
    }
}

bool ClientHandler::tryParseHeaderLine(const QByteArray &line)
{
    // Dinh dang: "ULD1 <fileName_percent_encoded> <fileSize>\n"
    QByteArray trimmed = line;
    while (trimmed.endsWith('\n') || trimmed.endsWith('\r'))
        trimmed.chop(1);

    const QList<QByteArray> parts = trimmed.split(' ');
    if (parts.size() != 3)
        return false;
    if (parts[0] != Protocol::MAGIC)
        return false;

    bool ok = false;
    const qint64 size = parts[2].toLongLong(&ok);
    if (!ok || size < 0)
        return false;

    const QString decodedName = QUrl::fromPercentEncoding(parts[1]);
    if (decodedName.isEmpty())
        return false;

    // Chi lay ten file, loai bo moi thong tin duong dan de tranh path traversal
    const QString safeName = QFileInfo(decodedName).fileName();
    if (safeName.isEmpty())
        return false;

    m_fileName = safeName;
    m_fileSize = size;

    const QString finalName = resolveDuplicateName(m_saveDir, m_fileName);
    m_finalPath = QDir(m_saveDir).filePath(finalName);
    m_tempPath = m_finalPath + QStringLiteral(".part");

    m_outFile.setFileName(m_tempPath);
    if (!m_outFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    m_state = State::ReadingData;
    emit logEvent(m_clientAddr, m_fileName, QStringLiteral("Dang nhan"),
                  QStringLiteral("Kich thuoc: %1 bytes").arg(m_fileSize));

    // Truong hop fileSize = 0 (file rong): hoan tat ngay
    if (m_fileSize == 0) {
        finishSuccess();
    }
    return true;
}

QString ClientHandler::resolveDuplicateName(const QString &dir, const QString &fileName) const
{
    // Quy tac xu ly file trung ten: neu file da ton tai, them hau to (1), (2), ...
    // truoc phan mo rong, vi du: report.pdf -> report (1).pdf
    QDir d(dir);
    if (!d.exists(fileName))
        return fileName;

    const QFileInfo fi(fileName);
    const QString base = fi.completeBaseName();
    const QString ext = fi.suffix();

    int counter = 1;
    QString candidate;
    do {
        candidate = ext.isEmpty()
                        ? QStringLiteral("%1 (%2)").arg(base).arg(counter)
                        : QStringLiteral("%1 (%2).%3").arg(base).arg(counter).arg(ext);
        ++counter;
    } while (d.exists(candidate));

    return candidate;
}

void ClientHandler::finishSuccess()
{
    m_state = State::Done;
    m_outFile.close();

    QFile::remove(m_finalPath); // phong truong hop race condition hiem gap
    if (!QFile::rename(m_tempPath, m_finalPath)) {
        emit logEvent(m_clientAddr, m_fileName, QStringLiteral("Loi"),
                      QStringLiteral("Khong the doi ten file tam sang file hoan chinh"));
        emit finishedHandling();
        m_socket->disconnectFromHost();
        return;
    }

    emit logEvent(m_clientAddr, m_fileName, QStringLiteral("Hoan tat"),
                  QDateTime::currentDateTime().toString(Qt::ISODate));
    emit finishedHandling();
    m_socket->disconnectFromHost();
}

void ClientHandler::failWithError(const QString &reason)
{
    m_state = State::Failed;
    if (m_outFile.isOpen())
        m_outFile.close();
    if (!m_tempPath.isEmpty())
        QFile::remove(m_tempPath); // xoa file tam do dang, khong cong nhan la file hoan chinh

    emit logEvent(m_clientAddr, m_fileName.isEmpty() ? QStringLiteral("(khong xac dinh)") : m_fileName,
                  QStringLiteral("Loi"), reason);
    emit finishedHandling();
    m_socket->disconnectFromHost();
}

void ClientHandler::onDisconnected()
{
    if (m_state == State::ReadingHeader || m_state == State::ReadingData) {
        // Client ngat ket noi dot ngot giua chung -> coi la loi, don dep file tam
        failWithError(QStringLiteral("Client ngat ket noi truoc khi truyen xong"));
    }
    m_socket->deleteLater();
}

void ClientHandler::onSocketError(QAbstractSocket::SocketError err)
{
    Q_UNUSED(err);
    if (m_state == State::ReadingHeader || m_state == State::ReadingData) {
        failWithError(m_socket->errorString());
    }
}
