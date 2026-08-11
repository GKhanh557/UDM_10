#pragma once

#include <QMainWindow>
#include <QMap>
#include <QVector>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QTableWidget;
class QProgressBar;
class QSpinBox;
class QLabel;
QT_END_NAMESPACE

class FileUploadTask;

// ClientWindow: cửa sổ chính phía Client.
// - Cho phép kéo-thả (drag & drop) hoặc chọn nhiều file vào hàng đợi.
// - Mỗi file có trạng thái riêng: Chờ / Đang tải lên / Hoàn tất / Lỗi.
// - Giới hạn số file upload đồng thời (cấu hình được), phần còn lại xếp hàng đợi.
// - Lỗi 1 file không làm dừng các file khác.
class ClientWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ClientWindow(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onAddFilesClicked();
    void onStartAllClicked();
    void onClearFinishedClicked();

private:
    struct FileRow {
        QString filePath;
        FileUploadTask *task = nullptr;
        QProgressBar *progressBar = nullptr;
        int rowIndex = -1;
        bool started = false;
        bool finished = false;
    };

    void setupUi();
    void addFileToQueue(const QString &filePath);
    void tryStartNextTasks();
    void startTaskForRow(int row);

    QLineEdit *m_hostEdit = nullptr;
    QLineEdit *m_portEdit = nullptr;
    QSpinBox *m_maxConcurrentSpin = nullptr;
    QPushButton *m_addFilesBtn = nullptr;
    QPushButton *m_startAllBtn = nullptr;
    QPushButton *m_clearFinishedBtn = nullptr;
    QLabel *m_dropHintLabel = nullptr;
    QTableWidget *m_table = nullptr;

    QVector<FileRow> m_rows;
    int m_runningCount = 0;
};
