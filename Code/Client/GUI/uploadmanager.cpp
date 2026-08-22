#include "uploadmanager.h"
#include "uploadworker.h"

#include <QFileInfo>

UploadManager::UploadManager(QObject *parent)
    : QObject(parent)
{
    m_pool.setMaxThreadCount(2);
}

void UploadManager::setServerAddress(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
}

void UploadManager::setMaxConcurrentUploads(int maxConcurrent)
{
    if (maxConcurrent > 0)
        m_pool.setMaxThreadCount(maxConcurrent);
}

int UploadManager::maxConcurrentUploads() const
{
    return m_pool.maxThreadCount();
}

void UploadManager::addFiles(const QStringList &filePaths)
{
    for (const QString &path : filePaths) {
        QFileInfo info(path);
        if (!info.exists() || !info.isFile())
            continue;

        emit fileQueued(path, info.size());
        emit fileStatus(path, QStringLiteral("Đang chờ"));

        auto *worker = new UploadWorker(path, m_host, m_port);
        m_activeWorkers[path] = worker;

        connect(worker, &UploadWorker::started, this, &UploadManager::fileStarted);
        connect(worker, &UploadWorker::progressChanged, this, &UploadManager::fileProgress);
        connect(worker, &UploadWorker::statusChanged, this, &UploadManager::fileStatus);
        connect(worker, &UploadWorker::finished, this,
                [this, path](const QString &filePath, bool success) {
                    m_activeWorkers.remove(filePath);
                    emit fileFinished(filePath, success);
                });
        
        m_pool.start(worker);
    }
}

void UploadManager::cancelFile(const QString &filePath)
{
    if (auto it = m_activeWorkers.find(filePath); it != m_activeWorkers.end() && *it) {
        (*it)->cancel();
    }
}
