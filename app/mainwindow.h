#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ModBtSrch;

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
    void btDiscovery(const QBluetoothDeviceInfo &device);
    void btError();
    void btDiscoverFinish();

private:
    Ui::MainWindow *ui;
    ModBtSrch *mod_btdiscovery;
    QBluetoothDeviceDiscoveryAgent *btDAgent;
};
#endif // MAINWINDOW_H
