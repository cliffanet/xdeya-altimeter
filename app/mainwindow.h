#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "wifidevicediscovery.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ModDevSrch;
class ModLogBook;
class NetProcess;

class QItemSelection;
class QBluetoothDeviceDiscoveryAgent;
class QBluetoothDeviceInfo;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum {
        pageDevSrch = 0,
        pageJmpList,
        pageJmpView
    };

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updState();

private slots:
    void on_btnBack_clicked();
    void on_btnReload_clicked();
    void on_btnView_clicked();
    void on_tvDevSrch_activated(const QModelIndex &index);
    void on_tvJmpList_activated(const QModelIndex &index);
    void on_btnJmpPrev_clicked();
    void on_btnJmpNext_clicked();

    void btDiscovery(const QBluetoothDeviceInfo &dev);
    void btError();
    void btDiscoverFinish();
    void wfDiscovery(const WifiDeviceItem &dev);
    void wfError(const QString &msg);
    void wfDiscoverFinish();

    void devConnect(qsizetype i);
    void jmpView(qsizetype i);

    void netWait();
    void netInit();
    void netAuth(bool ok);
    void netData(quint32 pos, quint32 max);

private:
    Ui::MainWindow *ui;
    ModDevSrch *mod_devsrch;
    ModLogBook *mod_logbook;
    NetProcess *netProc;
    QBluetoothDeviceDiscoveryAgent *btDAgent;
    WifiDeviceDiscovery *wfDAgent;
};
#endif // MAINWINDOW_H
