#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "wifidevicediscovery.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ModDevSrch;

class QItemSelection;
class QBluetoothDeviceDiscoveryAgent;
class QBluetoothDeviceInfo;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnDevSrch_clicked();
    void on_btnConnect_clicked();
    void on_tvDevSrch_activated(const QModelIndex &index);
    void devSrchSelect(const QItemSelection &, const QItemSelection &);
    void btDiscovery(const QBluetoothDeviceInfo &dev);
    void btError();
    void btDiscoverFinish();
    void wfDiscovery(const WifiDeviceItem &dev);
    void wfError(const QString &msg);
    void wfDiscoverFinish();
    void updDiscoverState();
    void devConnect(qsizetype i);

private:
    Ui::MainWindow *ui;
    ModDevSrch *mod_devsrch;
    QBluetoothDeviceDiscoveryAgent *btDAgent;
    WifiDeviceDiscovery *wfDAgent;
};
#endif // MAINWINDOW_H
