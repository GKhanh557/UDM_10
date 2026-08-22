#include "mainwindow.h"
#include "uploadmanager.h"

#include <QTableWidget>
#include <QProgressBar>
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QUrl>

namespace {
enum Column { ColFileName = 0, ColSize, ColProgress, ColSpeed, ColStatus, ColCount };
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);
    setWindowTitle(QStringLiteral("UDM_10 - Upload nhiều file"));
    resize(760, 420);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    auto *hint = new QLabel(QStringLiteral("Kéo-thả 1 hoặc nhiều file vào đây để upload"), central);
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet(QStringLiteral("padding: 12px; border: 2px dashed #888; color: #666;"));
    layout->addWidget(hint);

    m_table = new QTableWidget(0, ColCount, central);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Tên file"), QStringLiteral("Kích thước"),
        QStringLiteral("Tiến độ"), QStringLiteral("Tốc độ"), QStringLiteral("Trạng thái")
    });
    m_table->horizontalHeader()->setSectionResizeMode(ColFileName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColProgress, QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_table);

    setCentralWidget(central);

    m_manager = new UploadManager(this);
    m_manager->setServerAddress(QStringLiteral("127.0.0.1"), 5000); 
    m_manager->setMaxConcurrentUploads(2); 

    connect(m_manager, &UploadManager::fileQueued, this, &MainWindow::onFileQueued);
    connect(m_manager, &UploadManager::fileStatus, this, &MainWindow::onFileStatus);
    connect(m_manager, &UploadManager::fileProgress, this, &MainWindow::onFileProgress);
    connect(m_manager, &UploadManager::fileFinished, this, &MainWindow::onFileFinished);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QStringList paths;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile())
            paths << url.toLocalFile();
    }
    if (!paths.isEmpty())
        m_manager->addFiles(paths); 
}

int MainWindow::rowForFile(const QString &filePath) const
{
    return m_rowOfFile.value(filePath, -1);
}

void MainWindow::onFileQueued(const QString &filePath, qint64 fileSize)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    m_rowOfFile[filePath] = row;

    const QString name = QFileInfo(filePath).fileName();
    m_table->setItem(row, ColFileName, new QTableWidgetItem(name));
    m_table->setItem(row, ColSize, new QTableWidgetItem(formatBytes(fileSize)));

    auto *progress = new QProgressBar(m_table);
    progress->setRange(0, 100);
    progress->setValue(0);
    m_table->setCellWidget(row, ColProgress, progress);

    m_table->setItem(row, ColSpeed, new QTableWidgetItem(QStringLiteral("-")));
    m_table->setItem(row, ColStatus, new QTableWidgetItem(QStringLiteral("Đang chờ")));
}

void MainWindow::onFileStatus(const QString &filePath, const QString &status)
{
    const int row = rowForFile(filePath);
    if (row < 0) return;
    m_table->item(row, ColStatus)->setText(status);

    const bool isError = status.startsWith(QStringLiteral("Lỗi"));
    m_table->item(row, ColStatus)->setForeground(isError ? Qt::red : Qt::black);
}

void MainWindow::onFileProgress(const QString &filePath, qint64 sent, qint64 total, double speedBps)
{
    const int row = rowForFile(filePath);
    if (row < 0) return;

    if (auto *bar = qobject_cast<QProgressBar *>(m_table->cellWidget(row, ColProgress))) {
        const int percent = total > 0 ? static_cast<int>((sent * 100) / total) : 0;
        bar->setValue(percent);
    }
    m_table->item(row, ColSpeed)->setText(formatSpeed(speedBps));
}

void MainWindow::onFileFinished(const QString &filePath, bool success)
{
    const int row = rowForFile(filePath);
    if (row < 0) return;

    if (auto *bar = qobject_cast<QProgressBar *>(m_table->cellWidget(row, ColProgress))) {
        bar->setValue(success ? 100 : bar->value());
    }
}

QString MainWindow::formatBytes(qint64 bytes)
{
    static const char *units[] = {"B", "KB", "MB", "GB"};
    double size = bytes;
    int unit = 0;
    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        ++unit;
    }
    return QStringLiteral("%1 %2").arg(size, 0, 'f', 1).arg(units[unit]);
}

QString MainWindow::formatSpeed(double bytesPerSec)
{
    return formatBytes(static_cast<qint64>(bytesPerSec)) + QStringLiteral("/s");
}
