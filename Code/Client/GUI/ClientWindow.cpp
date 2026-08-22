#include "ClientWindow.h"
#include "FileUploadTask.h"

#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QProgressBar>
#include <QSpinBox>
#include <QLabel>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QFileInfo>
#include <QUrl>

namespace {
enum Column { ColFileName = 0, ColSize, ColProgress, ColSpeed, ColStatus, ColCount };

QString formatBytes(qint64 bytes)
{
    static const char *units[] = {"B", "KB", "MB", "GB"};
    double size = static_cast<double>(bytes);
    int unit = 0;
    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        ++unit;
    }
    return QStringLiteral("%1 %2").arg(size, 0, 'f', 1).arg(units[unit]);
}

QString formatSpeed(double bytesPerSec)
{
    return formatBytes(static_cast<qint64>(bytesPerSec)) + QStringLiteral("/s");
}
}

ClientWindow::ClientWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);
    setWindowTitle(QStringLiteral("UDM_10 - Upload nhiều file"));
    resize(820, 460);
    setupUi();
}

void ClientWindow::setupUi()
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);

    auto *configLayout = new QHBoxLayout();

    configLayout->addWidget(new QLabel(QStringLiteral("Server:"), central));
    m_hostEdit = new QLineEdit(QStringLiteral("127.0.0.1"), central);
    m_hostEdit->setMaximumWidth(140);
    configLayout->addWidget(m_hostEdit);

    configLayout->addWidget(new QLabel(QStringLiteral("Port:"), central));
    m_portEdit = new QLineEdit(QStringLiteral("5000"), central);
    m_portEdit->setMaximumWidth(70);
    configLayout->addWidget(m_portEdit);

    configLayout->addWidget(new QLabel(QStringLiteral("Số file upload cùng lúc:"), central));
    m_maxConcurrentSpin = new QSpinBox(central);
    m_maxConcurrentSpin->setRange(1, 10);
    m_maxConcurrentSpin->setValue(2);
    configLayout->addWidget(m_maxConcurrentSpin);

    configLayout->addStretch();

    m_addFilesBtn = new QPushButton(QStringLiteral("Thêm file..."), central);
    m_startAllBtn = new QPushButton(QStringLiteral("Bắt đầu tải"), central);
    m_clearFinishedBtn = new QPushButton(QStringLiteral("Xóa mục đã xong"), central);
    configLayout->addWidget(m_addFilesBtn);
    configLayout->addWidget(m_startAllBtn);
    configLayout->addWidget(m_clearFinishedBtn);

    mainLayout->addLayout(configLayout);

    m_dropHintLabel = new QLabel(QStringLiteral("Kéo-thả 1 hoặc nhiều file vào đây để thêm vào hàng đợi upload"), central);
    m_dropHintLabel->setAlignment(Qt::AlignCenter);
    m_dropHintLabel->setStyleSheet(QStringLiteral("padding: 10px; border: 2px dashed #888; color: #666;"));
    mainLayout->addWidget(m_dropHintLabel);

    m_table = new QTableWidget(0, ColCount, central);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Tên file"), QStringLiteral("Kích thước"),
        QStringLiteral("Tiến độ"), QStringLiteral("Tốc độ"), QStringLiteral("Trạng thái")
    });
    m_table->horizontalHeader()->setSectionResizeMode(ColFileName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColProgress, QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_table);

    setCentralWidget(central);

    connect(m_addFilesBtn, &QPushButton::clicked, this, &ClientWindow::onAddFilesClicked);
    connect(m_startAllBtn, &QPushButton::clicked, this, &ClientWindow::onStartAllClicked);
    connect(m_clearFinishedBtn, &QPushButton::clicked, this, &ClientWindow::onClearFinishedClicked);
}

void ClientWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void ClientWindow::dropEvent(QDropEvent *event)
{
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile())
            addFileToQueue(url.toLocalFile());
    }
}

void ClientWindow::onAddFilesClicked()
{
    const QStringList paths = QFileDialog::getOpenFileNames(this, QStringLiteral("Chọn file để upload"));
    for (const QString &path : paths)
        addFileToQueue(path);
}

void ClientWindow::addFileToQueue(const QString &filePath)
{
    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile())
        return;

    const int row = m_table->rowCount();
    m_table->insertRow(row);

    m_table->setItem(row, ColFileName, new QTableWidgetItem(info.fileName()));
    m_table->setItem(row, ColSize, new QTableWidgetItem(formatBytes(info.size())));

    auto *progressBar = new QProgressBar(m_table);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    m_table->setCellWidget(row, ColProgress, progressBar);

    m_table->setItem(row, ColSpeed, new QTableWidgetItem(QStringLiteral("-")));
    m_table->setItem(row, ColStatus, new QTableWidgetItem(QStringLiteral("Đang chờ")));

    FileRow fr;
    fr.filePath = filePath;
    fr.progressBar = progressBar;
    fr.rowIndex = row;
    fr.started = false;
    fr.finished = false;
    m_rows.push_back(fr);
}

void ClientWindow::onStartAllClicked()
{
    tryStartNextTasks();
}

void ClientWindow::tryStartNextTasks()
{
    const int maxConcurrent = m_maxConcurrentSpin->value();

    for (int i = 0; i < m_rows.size() && m_runningCount < maxConcurrent; ++i) {
        if (!m_rows[i].started && !m_rows[i].finished) {
            startTaskForRow(i);
        }
    }
}

void ClientWindow::startTaskForRow(int row)
{
    FileRow &fr = m_rows[row];
    fr.started = true;
    ++m_runningCount;

    const QString host = m_hostEdit->text().trimmed();
    const quint16 port = static_cast<quint16>(m_portEdit->text().toUInt());

    auto *task = new FileUploadTask(fr.filePath, host, port, this);
    fr.task = task;

    connect(task, &FileUploadTask::progressChanged, this,
            [this, row](qint64 sent, qint64 total, double speedBps) {
                if (row >= m_rows.size()) return;
                if (auto *bar = m_rows[row].progressBar) {
                    const int percent = total > 0 ? static_cast<int>((sent * 100) / total) : 0;
                    bar->setValue(percent);
                }
                if (auto *item = m_table->item(m_rows[row].rowIndex, ColSpeed))
                    item->setText(formatSpeed(speedBps));
            });

    connect(task, &FileUploadTask::statusChanged, this,
            [this, row](const QString &status) {
                if (row >= m_rows.size()) return;
                if (auto *item = m_table->item(m_rows[row].rowIndex, ColStatus)) {
                    item->setText(status);
                    // Lỗi của dòng này chỉ tô đỏ đúng dòng đó, không ảnh hưởng dòng khác.
                    item->setForeground(status.startsWith(QStringLiteral("Lỗi")) ? Qt::red : Qt::black);
                }
            });

    connect(task, &FileUploadTask::finished, this,
            [this, row](bool success) {
                if (row < m_rows.size()) {
                    m_rows[row].finished = true;
                    if (success && m_rows[row].progressBar)
                        m_rows[row].progressBar->setValue(100);
                    if (m_rows[row].task)
                        m_rows[row].task->deleteLater();
                    m_rows[row].task = nullptr;
                }
                --m_runningCount;
                tryStartNextTasks();
            });

    task->start();
}

void ClientWindow::onClearFinishedClicked()
{
    for (int i = m_rows.size() - 1; i >= 0; --i) {
        if (m_rows[i].finished) {
            m_table->removeRow(m_rows[i].rowIndex);
            m_rows.remove(i);
        }
    }
    // Cập nhật lại rowIndex cho các dòng còn lại vì removeRow() đã dịch hàng.
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows[i].rowIndex = i;
    }
}
