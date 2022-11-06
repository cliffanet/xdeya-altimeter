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

    updState();
    ui->labState->setText("");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updState()
{
    ui->btnBack->setVisible(ui->stackWnd->currentIndex() > pageDevSrch);

    ui->btnConnect->setVisible(ui->stackWnd->currentIndex() == pageDevSrch);
    ui->btnConnect->setEnabled(
        (ui->stackWnd->currentIndex() == pageDevSrch) &&
        (ui->tvDevSrch->selectionModel()->selection().count() > 0)
    );

    switch (ui->stackWnd->currentIndex()) {
        case pageDevSrch: {
            bool active =
                    btDAgent->isActive() ||
                    wfDAgent->isActive();

            ui->progRcvData->setVisible(active);
            ui->progRcvData->setRange(0, 0);
            ui->btnReload->setEnabled(!active);
            break;
        }

        default:
            ui->progRcvData->setRange(0, 0);
            ui->progRcvData->setVisible(netProc->wait() > NetProcess::wtUnknown);
            ui->btnReload->setEnabled(netProc->wait() == NetProcess::wtUnknown);
    }
}


void MainWindow::on_btnBack_clicked()
{
    switch (ui->stackWnd->currentIndex()) {
        case pageJmpList:
            ui->stackWnd->setCurrentIndex(pageDevSrch);
            break;
    }
    updState();
}

void MainWindow::on_btnReload_clicked()
{
    switch (ui->stackWnd->currentIndex()) {
        case pageDevSrch:
            mod_devsrch->clear();
            //btDAgent->start();
            wfDAgent->start();
            break;
    }
    ui->labState->setText("");
    updState();
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
    ui->labState->setText( btDAgent->errorString() );
    updState();
}

void MainWindow::btDiscoverFinish()
{
    qDebug() << "BT Found finish()";
    updState();
}

void MainWindow::wfDiscovery(const WifiDeviceItem &dev)
{
    qDebug() << "Wifi Found new device:" << dev.name << '(' << dev.ip.toString() << ':' << dev.port << ')';
    mod_devsrch->addWifi(dev);
}

void MainWindow::wfError(const QString &msg)
{
    ui->labState->setText( msg );
    updState();
}

void MainWindow::wfDiscoverFinish()
{
    qDebug() << "Wifi Found finish()";
    updState();
}

void MainWindow::devConnect(qsizetype i)
{
    if (btDAgent->isActive())
        btDAgent->stop();
    if (wfDAgent->isActive())
        wfDAgent->stop();

    ui->labState->setText("");
    ui->stackWnd->setCurrentIndex(pageJmpList);

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

    updState();
}

void MainWindow::netWait()
{
    switch (netProc->wait()) {
        case NetProcess::wtDisconnected:
            switch (netProc->err()) {
                case NetProcess::errAuth:
                    ui->labState->setText("Неверный код авторизации");
                    break;

                case NetProcess::errProto:
                    ui->labState->setText("Ошибка протокола");
                    break;

                default:
                    ui->labState->setText("Соединение прервано");
            }
            break;
        case NetProcess::wtInit:
            ui->labState->setText("Ожидание подключения");
            break;

        default:
            ui->labState->setText("");
    }
    updState();
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
        ui->stackWnd->setCurrentIndex(pageDevSrch);
    }
    updState();
}

