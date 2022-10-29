#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "modbtsrch.h""

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>

/*  В Info.plist добавить:
 *
    <key>NSBluetoothAlwaysUsageDescription</key>
    <string>App would like to use your bluetooth for communication purposes</string>
    <key>NSBluetoothPeripheralUsageDescription</key>
    <string>App would like to use your bluetooth for communication purposes</string>

 */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("Xde-Ya"));

    ui->progressBar->setVisible(false);
    ui->progressBar->setRange(0, 0);
    ui->labErr->setVisible(false);

    mod_btdiscovery = new ModBtSrch(this);
    ui->tvBT->setModel(mod_btdiscovery);

    btDAgent = new QBluetoothDeviceDiscoveryAgent(this);
    btDAgent->setLowEnergyDiscoveryTimeout(10000);
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,    this, &MainWindow::btDiscovery);
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,       this, &MainWindow::btError);
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::finished,            this, &MainWindow::btDiscoverFinish);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_btnBlueSrch_clicked()
{
    ui->labErr->setVisible(false);
    btDAgent->start();
    if (btDAgent->isActive()) {
        ui->progressBar->setVisible(true);
        ui->btnBlueSrch->setEnabled(false);
    }
}

void MainWindow::btDiscovery(const QBluetoothDeviceInfo &device)
{
    qDebug() << "Found new device:" << device.name() << '(' << device.address().toString() << ')';
    mod_btdiscovery->update(btDAgent->discoveredDevices());
}

void MainWindow::btError()
{
    ui->labErr->setText( btDAgent->errorString() );
    ui->labErr->setVisible(true);
    btDiscoverFinish();
}

void MainWindow::btDiscoverFinish()
{
    qDebug() << "Found finish()";
    ui->progressBar->setVisible(false);
    ui->btnBlueSrch->setEnabled(true);
}

