#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "moddevsrch.h"
#include "modlogbook.h"
#include "netprocess.h"
#include "formauth.h"
#include "trkbutton.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QItemSelectionModel>

#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QFile>
#include <QTemporaryFile>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      m_ftmp(nullptr)
{
    ui->setupUi(this);
    setWindowTitle(tr("Xde-Ya"));

    m_lbinfrows = ui->jmpInfLayout->rowCount();

    netProc = new NetProcess(this);
    connect(netProc, &NetProcess::waitChange,   this, &MainWindow::netWait);
    connect(netProc, &NetProcess::rcvInit,      this, &MainWindow::netInit);
    connect(netProc, &NetProcess::rcvAuth,      this, &MainWindow::netAuth);
    connect(netProc, &NetProcess::rcvData,      this, &MainWindow::netData);
    connect(netProc, &NetProcess::rcvLogBook,   this, &MainWindow::netLogBook);
    connect(netProc, &NetProcess::rcvTrack,     this, &MainWindow::netTrack);
    connect(netProc, &NetProcess::rcvTrkMapCenter, this, &MainWindow::netTrkMapCenter);

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

    mod_devsrch = new ModDevSrch(this);
    ui->tvDevSrch->setModel(mod_devsrch);
    connect(ui->tvDevSrch->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updState);

    mod_logbook = new ModLogBook(netProc->logbook(), this);
    ui->tvJmpList->setModel(mod_logbook);
    connect(ui->tvJmpList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updState);

    ui->stackWnd->setCurrentIndex(pageDevSrch);
    updState();
    ui->labState->setText("");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updState()
{
    ui->btnBack->setVisible( ui->stackWnd->currentIndex() > pageDevSrch );
    ui->btnReload->setVisible( ui->stackWnd->currentIndex() != pageJmpView );

    switch (ui->stackWnd->currentIndex()) {
        case pageDevSrch:
            ui->btnView->setVisible(true);
            ui->btnView->setEnabled(
                ui->tvDevSrch->selectionModel()->selection().count() > 0
            );
            break;

        case pageJmpList:
            ui->btnView->setVisible(true);
            ui->btnView->setEnabled(
                ui->tvJmpList->selectionModel()->selection().count() > 0
            );
            break;

        default:
            ui->btnView->setVisible(false);
    }

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
            if (
                    (netProc->wait() != NetProcess::wtLogBook) &&
                    (netProc->wait() != NetProcess::wtTrack)
                )
                ui->progRcvData->setRange(0, 0);
            ui->progRcvData->setVisible(netProc->wait() > NetProcess::wtUnknown);
            ui->btnReload->setEnabled(netProc->wait() == NetProcess::wtUnknown);
    }
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

void MainWindow::jmpView(qsizetype i)
{
    auto &jmp = netProc->logbook(i);

    QDateTime dt(QDate(jmp.tm.year, jmp.tm.mon, jmp.tm.day), QTime(jmp.tm.h, jmp.tm.m, jmp.tm.s));
    qDebug() << "View jump: " << jmp.num << " (" << dt.toString("d.MM.yyyy hh:mm:ss") << ")";

    ui->lvJmpNumber->setText(QString::number(jmp.num));

    if (jmp.toff.tmoffset) {
        QDateTime dtbeg = dt.addMSecs(-1*jmp.toff.tmoffset);
        ui->lvJmpDTTakeoff->setText(dtbeg.toString("d.MM.yyyy hh:mm"));
    }
    else {
        ui->lvJmpDTTakeoff->setText("");
    }
    ui->lvJmpDTBegin  ->setText(dt.toString("d.MM.yyyy hh:mm"));
    ui->lvJmpAltBegin ->setText(QString::number(jmp.beg.alt) + " m");

    char s[64];
    uint hour, min, sec;
    sec = (jmp.cnp.tmoffset - jmp.beg.tmoffset) / 1000;
    min = sec / 60;
    sec -= min*60;
    hour = min / 60;
    min -= hour*60;
    snprintf(s, sizeof(s), "%d:%02d:%02d", hour, min, sec);
    ui->lvJmpTimeFF   ->setText(QString(s));

    ui->lvJmpAltCnp   ->setText(QString::number(jmp.cnp.alt) + " m");

    sec = (jmp.end.tmoffset - jmp.cnp.tmoffset) / 1000;
    min = sec / 60;
    sec -= min*60;
    hour = min / 60;
    min -= hour*60;
    snprintf(s, sizeof(s), "%d:%02d:%02d", hour, min, sec);
    ui->lvJmpTimeCnp  ->setText(QString(s));

    while (ui->jmpInfLayout->rowCount() > m_lbinfrows)
        ui->jmpInfLayout->removeRow(m_lbinfrows);
    for (const auto &trk : netProc->trklist()) {
        if (trk.jmpnum != jmp.num)
            continue;
        QDateTime dt(QDate(trk.tmbeg.year, trk.tmbeg.mon, trk.tmbeg.day), QTime(trk.tmbeg.h, trk.tmbeg.m, trk.tmbeg.s));
        qDebug() << "trk: " << trk.jmpkey << " -- " << jmp.key << " (" << dt.toString("d.MM.yyyy hh:mm:ss") << ")";

        ui->jmpInfLayout->addRow(tr("Трэк:"), new TrkButton(dt.toString("d.MM.yyyy hh:mm:ss"), trk, this));
    }

    ui->btnJmpPrev->setEnabled(i > 0);
    ui->btnJmpNext->setEnabled(i < (netProc->logbook().size()-1));

    ui->tvJmpList->setCurrentIndex(ui->tvJmpList->model()->index(i, 0));

    ui->stackWnd->setCurrentIndex(pageJmpView);
    updState();
}

void MainWindow::trkView(const trklist_item_t &trk)
{
    QDateTime dt(QDate(trk.tmbeg.year, trk.tmbeg.mon, trk.tmbeg.day), QTime(trk.tmbeg.h, trk.tmbeg.m, trk.tmbeg.s));
    qDebug() << "trk view: " << trk.jmpnum << " -- " << trk.jmpkey << " (" << dt.toString("d.MM.yyyy hh:mm:ss") << ")";

    ui->wvTrack->setHtml("");
    netProc->requestTrack(trk);

    ui->stackWnd->setCurrentIndex(pageTrkView);
    updState();
}


void MainWindow::on_btnBack_clicked()
{
    switch (ui->stackWnd->currentIndex()) {
        case pageJmpList:
            ui->stackWnd->setCurrentIndex(pageDevSrch);
            break;

        case pageJmpView:
            ui->stackWnd->setCurrentIndex(pageJmpList);
            break;

        case pageTrkView:
            ui->stackWnd->setCurrentIndex(pageJmpView);
            break;
    }
    updState();
}

void MainWindow::on_btnReload_clicked()
{
    ui->labState->setText("");
    switch (ui->stackWnd->currentIndex()) {
        case pageDevSrch:
            mod_devsrch->clear();
            //btDAgent->start();
            wfDAgent->start();
            break;

        case pageJmpList:
            netProc->requestLogBook();
            break;

        case pageTrkView:
            //ui->wvTrack->reload();
            {
        QString cmd("loadGPX('" + netProc->httpAddr() +"/track.gpx');");
        qDebug() << "cmd1: " << cmd;
        ui->wvTrack->page()->runJavaScript(cmd);
            }
            break;
    }
    updState();
}

void MainWindow::on_btnView_clicked()
{
    switch (ui->stackWnd->currentIndex()) {
        case pageDevSrch: {
            auto &sel = ui->tvDevSrch->selectionModel()->selection();
            if (sel.count() > 0)
                devConnect(sel.indexes().first().row());
            break;
        }

        case pageJmpList: {
            auto &sel = ui->tvJmpList->selectionModel()->selection();
            if (sel.count() > 0)
                jmpView(sel.indexes().first().row());
            break;
        }
    }
}

void MainWindow::on_tvDevSrch_activated(const QModelIndex &index)
{
    devConnect(index.row());
}

void MainWindow::on_tvJmpList_activated(const QModelIndex &index)
{
    jmpView(index.row());
}

void MainWindow::on_btnJmpPrev_clicked()
{
    auto &sel = ui->tvJmpList->selectionModel()->selection();
    if (sel.count() == 0)
        return;
    int row = sel.indexes().first().row();
    if (row > 0)
        jmpView(row-1);
}

void MainWindow::on_btnJmpNext_clicked()
{
    auto &sel = ui->tvJmpList->selectionModel()->selection();
    if (sel.count() == 0)
        return;
    int row = sel.indexes().first().row();
    if (row+1 < netProc->logbook().size())
        jmpView(row+1);
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

        case NetProcess::wtLogBookBeg:
        case NetProcess::wtLogBook:
            ui->labState->setText("Приём логбука");
            break;

        case NetProcess::wtTrkListBeg:
        case NetProcess::wtTrkList:
            ui->labState->setText("Приём списка треков");
            break;

        case NetProcess::wtTrackBeg:
        case NetProcess::wtTrack:
            ui->labState->setText("Приём трека");
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

void MainWindow::netAuth(bool ok)
{
    if (!ok)
        return;
    netProc->requestLogBook();
}

void MainWindow::netData(quint32 pos, quint32 max)
{
    ui->progRcvData->setRange(0, max);
    if (max > 0)
        ui->progRcvData->setValue(pos);

    switch (netProc->wait()) {
        case NetProcess::wtLogBook:
            emit mod_logbook->layoutChanged();
            ui->tvJmpList->scrollToBottom();
            break;
    }
}

void MainWindow::netLogBook()
{
    netProc->requestTrackList();
}

void MainWindow::netTrack()
{
    if (!netProc->trkMapCenter()) {
        ui->labState->setText("Нет связи со спутниками");
        return;
    }

    if (m_ftmp != nullptr)
        delete m_ftmp;
    m_ftmp = new QTemporaryFile(QDir::tempPath() + "/trackXXXXXX.gpx", this);
    if (!m_ftmp->open()) {
        qDebug() << "can't open tmp-file: " << m_ftmp->fileName();
        return;
    }
    qDebug() << "Tmp file: " << m_ftmp->fileName();
    if (!netProc->trkSaveGPX(*m_ftmp)) {
        qDebug() << "fail write GPX";
        return;
    }
    m_ftmp->close();
    //QString cmd("loadGPX('file://"+m_ftmp->fileName()+"');");
    QString cmd("loadGPX('" + netProc->httpAddr() +"/track.gpx');");
    qDebug() << "cmd: " << cmd;
    ui->wvTrack->page()->runJavaScript(cmd);
}

void MainWindow::netTrkMapCenter(const log_item_t &ti)
{
    QString center(QString::number(static_cast<double>(ti.lat)/10000000) + tr(", ") + QString::number(static_cast<double>(ti.lon)/10000000));
    qDebug() << "netTrkMapCenter: " << center;

    QString data;
    QFile ftxt(QString(":/navi.html"));
    if (!ftxt.open(QIODevice::ReadOnly)) {
        qDebug() << "file(" << ftxt.fileName() << ") not opened";
        return;
    }
    data = ftxt.readAll();
    ftxt.close();
    data.replace(QString("%MAP_CENTER%"), center);

    ui->wvTrack->setHtml(data);
}

