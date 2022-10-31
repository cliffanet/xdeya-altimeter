#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ModBtSrch;

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
    void on_btnBlueSrch_clicked();
    void on_btnConnect_clicked();
    void on_tvBT_activated(const QModelIndex &index);
    void btSrchSelect(const QItemSelection &, const QItemSelection &);
    void btDiscovery(const QBluetoothDeviceInfo &device);
    void btError();
    void btDiscoverFinish();
    void btConnect(qsizetype i);

private:
    Ui::MainWindow *ui;
    ModBtSrch *mod_btdiscovery;
    QBluetoothDeviceDiscoveryAgent *btDAgent;
};
#endif // MAINWINDOW_H
