#pragma once

#include <QMainWindow>
#include <QMap>

class QTableWidget;
class QProgressBar;
class UploadManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
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

    QMap<QString, int> m_rowOfFile;

    int rowForFile(const QString &filePath) const;
    static QString formatBytes(qint64 bytes);
    static QString formatSpeed(double bytesPerSec);
};
