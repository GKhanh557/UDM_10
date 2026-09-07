#pragma once

#include <QMainWindow>
#include <QTcpServer>
#include <QSet>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QTableWidget;
class QLabel;
QT_END_NAMESPACE

class ClientHandler;

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServerWindow(QWidget *parent = nullptr);

private slots:
    void onStartStopClicked();
    void onBrowseClicked();
    void onNewConnection();
    void onHandlerLog(const QString &clientAddr, const QString &fileName,
                       const QString &status, const QString &detail);

private:
    void setupUi();
    void appendLogRow(const QString &time, const QString &clientAddr,
                       const QString &fileName, const QString &status, const QString &detail);

    QTcpServer *m_server = nullptr;

    QLineEdit *m_portEdit = nullptr;
    QLineEdit *m_dirEdit = nullptr;
    QPushButton *m_browseBtn = nullptr;
    QPushButton *m_startStopBtn = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTableWidget *m_logTable = nullptr;

    QSet<ClientHandler *> m_activeHandlers;
};
