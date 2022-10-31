#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "modbtsrch.h"

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
    ui->labErr->setVisible(false);

    mod_btdiscovery = new ModBtSrch(this);
    ui->tvBT->setModel(mod_btdiscovery);
    connect(
        ui->tvBT->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        SLOT(btSrchSelect(const QItemSelection &, const QItemSelection &))
    );

    btDAgent = new QBluetoothDeviceDiscoveryAgent(this);
    btDAgent->setLowEnergyDiscoveryTimeout(10000);
    qDebug() << btDAgent->supportedDiscoveryMethods();
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


void MainWindow::on_btnConnect_clicked()
{
    auto &sel = ui->tvBT->selectionModel()->selection();
    if (sel.count() > 0)
        btConnect(sel.indexes().first().row());
}

void MainWindow::on_tvBT_activated(const QModelIndex &index)
{
    btConnect(index.row());
}

void MainWindow::btSrchSelect(const QItemSelection &, const QItemSelection &)
{
    int cnt = ui->tvBT->selectionModel()->selection().count();
    ui->btnConnect->setEnabled(cnt > 0);
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

void MainWindow::btConnect(qsizetype i)
{
    if (btDAgent->isActive()) {
        btDAgent->stop();
        btDiscoverFinish();
    }

    auto dev = mod_btdiscovery->device(i);
    qDebug() << "Connect to: " << dev.name() << " (" << dev.address().toString() << ")";

    ui->stackWnd->setCurrentIndex(1);
}

