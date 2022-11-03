#ifndef MODDEVSRCH_H
#define MODDEVSRCH_H

#include <QAbstractTableModel>
//#include <QBluetoothDeviceInfo>

#include "wifidevicediscovery.h"

class ModDevSrch : public QAbstractTableModel
{
    Q_OBJECT

public:
    typedef enum {
        SRS_UNKNOWN,
        SRC_WIFI,
        SRC_BLUETOOTH
    } src_t;
    typedef struct {
        src_t                   src;
        QString                 name;
        //QBluetoothDeviceInfo    bt;
        WifiDeviceItem          wifi;
    } CItem;
    typedef QList<CItem> CList;

    ModDevSrch(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void clear();
    void add(const CItem &item);
    void addWifi(const WifiDeviceItem &dev);
    QString name(qsizetype i) const;
    src_t src(qsizetype i) const;

private:
    CList m_list;

};

#endif // MODDEVSRCH_H
