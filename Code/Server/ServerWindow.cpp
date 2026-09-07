#include "ServerWindow.h"
#include "ClientHandler.h"
#include "ProtocolCommon.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QIntValidator>

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_server(new QTcpServer(this))
{
    setupUi();
    connect(m_server, &QTcpServer::newConnection, this, &ServerWindow::onNewConnection);
}

void ServerWindow::setupUi()
{
    setWindowTitle(QStringLiteral("UDM_10 - Server (Upload nhieu file)"));
    resize(820, 480);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);

    // --- Khu vực cấu hình ---
    auto *form = new QFormLayout();

    m_portEdit = new QLineEdit(QString::number(Protocol::DEFAULT_PORT), this);
    m_portEdit->setValidator(new QIntValidator(1, 65535, this));
    form->addRow(QStringLiteral("Port:"), m_portEdit);

    auto *dirLayout = new QHBoxLayout();
    m_dirEdit = new QLineEdit(QDir(QDir::currentPath()).filePath(QStringLiteral("received_files")), this);
    m_browseBtn = new QPushButton(QStringLiteral("Chon thu muc..."), this);
    dirLayout->addWidget(m_dirEdit);
    dirLayout->addWidget(m_browseBtn);
    form->addRow(QStringLiteral("Thu muc luu file:"), dirLayout);

    rootLayout->addLayout(form);

    // --- Nút Start/Stop + trạng thái ---
    auto *controlLayout = new QHBoxLayout();
    m_startStopBtn = new QPushButton(QStringLiteral("Bat dau (Start)"), this);
    m_statusLabel = new QLabel(QStringLiteral("Server dang dung"), this);
    controlLayout->addWidget(m_startStopBtn);
    controlLayout->addWidget(m_statusLabel);
    controlLayout->addStretch();
    rootLayout->addLayout(controlLayout);

    // --- Bảng log ---
    m_logTable = new QTableWidget(0, 5, this);
    m_logTable->setHorizontalHeaderLabels(
        {QStringLiteral("Thoi gian"), QStringLiteral("Client"), QStringLiteral("File"),
         QStringLiteral("Trang thai"), QStringLiteral("Chi tiet")});
    m_logTable->horizontalHeader()->setStretchLastSection(true);
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    rootLayout->addWidget(m_logTable);

    setCentralWidget(central);

    connect(m_startStopBtn, &QPushButton::clicked, this, &ServerWindow::onStartStopClicked);
    connect(m_browseBtn, &QPushButton::clicked, this, &ServerWindow::onBrowseClicked);
}

void ServerWindow::onBrowseClicked()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Chon thu muc luu file"));
    if (!dir.isEmpty())
        m_dirEdit->setText(dir);
}

void ServerWindow::onStartStopClicked()
{
    if (m_server->isListening()) {
        m_server->close();
        m_startStopBtn->setText(QStringLiteral("Bat dau (Start)"));
        m_statusLabel->setText(QStringLiteral("Server dang dung"));
        m_portEdit->setEnabled(true);
        m_dirEdit->setEnabled(true);
        m_browseBtn->setEnabled(true);
        return;
    }

    const quint16 port = static_cast<quint16>(m_portEdit->text().toUInt());
    if (!m_server->listen(QHostAddress::Any, port)) {
        QMessageBox::warning(this, QStringLiteral("Loi"),
                              QStringLiteral("Khong the mo port %1: %2")
                                  .arg(port).arg(m_server->errorString()));
        return;
    }

    QDir().mkpath(m_dirEdit->text());

    m_startStopBtn->setText(QStringLiteral("Dung (Stop)"));
    m_statusLabel->setText(QStringLiteral("Dang lang nghe tren port %1 — Thu muc: %2")
                                .arg(port).arg(m_dirEdit->text()));
    m_portEdit->setEnabled(false);
    m_dirEdit->setEnabled(false);
    m_browseBtn->setEnabled(false);
}

void ServerWindow::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();

        auto *handler = new ClientHandler(socket, m_dirEdit->text(), this);
        m_activeHandlers.insert(handler);

        connect(handler, &ClientHandler::logEvent, this, &ServerWindow::onHandlerLog);
        connect(handler, &ClientHandler::finishedHandling, this, [this, handler]() {
            m_activeHandlers.remove(handler);
            handler->deleteLater();
        });
    }
}

void ServerWindow::onHandlerLog(const QString &clientAddr, const QString &fileName,
                                 const QString &status, const QString &detail)
{
    const QString time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss dd/MM/yyyy"));
    appendLogRow(time, clientAddr, fileName, status, detail);
}

void ServerWindow::appendLogRow(const QString &time, const QString &clientAddr,
                                  const QString &fileName, const QString &status, const QString &detail)
{
    const int row = m_logTable->rowCount();
    m_logTable->insertRow(row);
    m_logTable->setItem(row, 0, new QTableWidgetItem(time));
    m_logTable->setItem(row, 1, new QTableWidgetItem(clientAddr));
    m_logTable->setItem(row, 2, new QTableWidgetItem(fileName));
    m_logTable->setItem(row, 3, new QTableWidgetItem(status));
    m_logTable->setItem(row, 4, new QTableWidgetItem(detail));
    m_logTable->scrollToBottom();
}
