#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "moddevsrch.h"
#include "netprocess.h"
#include "formauth.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("Xde-Ya"));

    ui->progressBar->setVisible(false);
    ui->progressBar->setRange(0, 0);
    ui->labSrchErr->setVisible(false);

    mod_devsrch = new ModDevSrch(this);
    ui->tvDevSrch->setModel(mod_devsrch);
    connect(
        ui->tvDevSrch->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        SLOT(devSrchSelect(const QItemSelection &, const QItemSelection &))
    );

    netProc = new NetProcess(this);
    connect(netProc, &NetProcess::waitChange,   this, &MainWindow::netWait);
    connect(netProc, &NetProcess::rcvInit,      this, &MainWindow::netInit);

    btDAgent = new QBluetoothDeviceDiscoveryAgent(this);
    btDAgent->setLowEnergyDiscoveryTimeout(10000);
    qDebug() << btDAgent->supportedDiscoveryMethods();
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,    this, &MainWindow::btDiscovery);
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,       this, &MainWindow::btError);
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::finished,            this, &MainWindow::btDiscoverFinish);

    wfDAgent = new WifiDeviceDiscovery(this);
    connect(wfDAgent, &WifiDeviceDiscovery::discoverDeviceFound,    this, &MainWindow::wfDiscovery);
    connect(wfDAgent, &WifiDeviceDiscovery::onError,                this, &MainWindow::wfError);
    connect(wfDAgent, &WifiDeviceDiscovery::discoverFinish,         this, &MainWindow::wfDiscoverFinish);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_btnDevSrch_clicked()
{
    ui->labSrchErr->setVisible(false);
    mod_devsrch->clear();
    //btDAgent->start();
    wfDAgent->start();
    updDiscoverState();
}


void MainWindow::on_btnConnect_clicked()
{
    auto &sel = ui->tvDevSrch->selectionModel()->selection();
    if (sel.count() > 0)
        devConnect(sel.indexes().first().row());
}

void MainWindow::on_tvDevSrch_activated(const QModelIndex &index)
{
    devConnect(index.row());
}

void MainWindow::on_btnBackJmp_clicked()
{
    ui->stackWnd->setCurrentIndex(0);
}

void MainWindow::devSrchSelect(const QItemSelection &, const QItemSelection &)
{
    int cnt = ui->tvDevSrch->selectionModel()->selection().count();
    ui->btnConnect->setEnabled(cnt > 0);
}

void MainWindow::btDiscovery(const QBluetoothDeviceInfo &dev)
{
    qDebug() << "BT Found new device:" << dev.name() << '(' << dev.address().toString() << ')';
    /*
    if (
            dev.isValid() &&
            !dev.address().isNull() &&
            (dev.name() != tr(""))
            //(dev.address().toString() != tr("00:00:00:00:00:00") )
        )
        mod_devsrch->addBluetooth(dev);
    */
}

void MainWindow::btError()
{
    ui->labSrchErr->setText( btDAgent->errorString() );
    ui->labSrchErr->setVisible(true);
    updDiscoverState();
}

void MainWindow::btDiscoverFinish()
{
    qDebug() << "BT Found finish()";
    updDiscoverState();
}

void MainWindow::wfDiscovery(const WifiDeviceItem &dev)
{
    qDebug() << "Wifi Found new device:" << dev.name << '(' << dev.ip.toString() << ':' << dev.port << ')';
    mod_devsrch->addWifi(dev);
}

void MainWindow::wfError(const QString &msg)
{
    ui->labSrchErr->setText( msg );
    ui->labSrchErr->setVisible(true);
    updDiscoverState();
}

void MainWindow::wfDiscoverFinish()
{
    qDebug() << "Wifi Found finish()";
    updDiscoverState();
}

void MainWindow::updDiscoverState()
{
    bool active =
            btDAgent->isActive() ||
            wfDAgent->isActive();
    ui->progressBar->setVisible(active);
    ui->btnDevSrch->setEnabled(!active);
}

void MainWindow::devConnect(qsizetype i)
{
    if (btDAgent->isActive())
        btDAgent->stop();
    if (wfDAgent->isActive())
        wfDAgent->stop();
    updDiscoverState();

    qDebug() << "Connect to: " << mod_devsrch->name(i) << " (" << mod_devsrch->src(i) << ")";
    switch (mod_devsrch->src(i)) {
        case ModDevSrch::SRC_WIFI: {
            auto &wifi = mod_devsrch->wifi(i);
            netProc->connectTcp(wifi.ip, wifi.port);
            break;
        }

        default:
            return;
    }

    ui->stackWnd->setCurrentIndex(1);
}

void MainWindow::netWait()
{
    switch (netProc->wait()) {
        case NetProcess::wtDisconnected:
            switch (netProc->err()) {
                case NetProcess::errAuth:
                    ui->labRcvState->setText("Неверный код авторизации");
                    break;

                case NetProcess::errProto:
                    ui->labRcvState->setText("Ошибка протокола");
                    break;

                default:
                    ui->labRcvState->setText("Соединение прервано");
            }
            break;
        case NetProcess::wtInit:
            ui->labRcvState->setText("Ожидание подключения");
            ui->progRcvData->setRange(0, 0);
            break;

        default:
            ui->labRcvState->setText("");
    }

    ui->progRcvData->setVisible(netProc->wait() > NetProcess::wtUnknown);
}

void MainWindow::netInit()
{
    FormAuth f;

    uint16_t code =
        f.exec() == QDialog::Accepted ?
                f.getCode() :
                0;

    if (code > 0) {
        netProc->requestAuth(code);
    }
    else {
        netProc->disconnect();
        ui->stackWnd->setCurrentIndex(0);
    }
}

