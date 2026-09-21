#pragma once
#include <QMainWindow>
#include <QTcpSocket>
#include <QFile>
#include <QMap>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QTableWidget;
class QProgressBar;
QT_END_NAMESPACE

// Số file được phép upload cùng lúc tối đa (đề bài yêu cầu phải công bố rõ số này)
const int MAX_CONCURRENT_UPLOADS = 2;

// Thời gian chờ tối đa khi kết nối tới Server, tránh treo vô hạn nếu Server không phản hồi
const int CONNECT_TIMEOUT_MS = 5000;

// Struct nhỏ lưu thông tin 1 file đang được upload qua socket riêng
struct UploadInfo {
    QString filePath;
    qint64 fileSize = 0;
    qint64 sent = 0;
    QFile *file = nullptr;
    int row = -1; // dòng tương ứng trong bảng

    // Dùng để tính tốc độ upload (KB/s), cập nhật mỗi ~500ms
    qint64 lastTickMs = 0;
    qint64 sentSinceLastTick = 0;
};

class ClientWindow : public QMainWindow
{
    Q_OBJECT
public:
    ClientWindow(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void chooseFiles();
    void uploadAll();
    void socketConnected();
    void socketBytesWritten(qint64 bytes);
    void socketError();

private:
    QLineEdit *hostEdit;
    QLineEdit *portEdit;
    QPushButton *chooseBtn;
    QPushButton *uploadBtn;
    QTableWidget *table;

    QStringList pendingFiles; // các file đang chờ (chưa được cấp slot upload)
    QList<int> pendingRows;   // dòng tương ứng với từng file trong pendingFiles
    QMap<QTcpSocket*, UploadInfo> uploads;
    int runningCount = 0;     // số file đang upload đồng thời hiện tại

    void addFileRow(const QString &path);
    void tryStartNext();      // lấy file tiếp theo trong hàng đợi ra upload, nếu còn slot trống
    void startUpload(const QString &path, int row);
    void sendNextChunk(QTcpSocket *socket);
    void failUpload(QTcpSocket *socket, const QString &reason); // dùng chung khi lỗi hoặc timeout
};
