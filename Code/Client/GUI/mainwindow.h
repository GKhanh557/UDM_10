#pragma once

#include <QMainWindow>
#include <QMap>

class QTableWidget;
class QProgressBar;
class UploadManager;

// Cửa sổ chính: khu vực kéo-thả file + bảng theo dõi trạng thái/tiến độ
// từng file (chờ / đang tải / hoàn tất / lỗi), tốc độ upload riêng từng file.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    // Bật kéo-thả: chấp nhận 1 hoặc nhiều file cùng lúc
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onFileQueued(const QString &filePath, qint64 fileSize);
    void onFileStatus(const QString &filePath, const QString &status);
    void onFileProgress(const QString &filePath, qint64 sent, qint64 total, double speedBps);
    void onFileFinished(const QString &filePath, bool success);

private:
    QTableWidget *m_table;
    UploadManager *m_manager;

    // Ánh xạ đường dẫn file -> số dòng trong bảng
    QMap<QString, int> m_rowOfFile;

    int rowForFile(const QString &filePath) const;
    static QString formatBytes(qint64 bytes);
    static QString formatSpeed(double bytesPerSec);
};
