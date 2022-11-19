#ifndef DEVINFO_H
#define DEVINFO_H

#include <QObject>
#include <QBluetoothDeviceInfo>

#include "wifidevicediscovery.h"

class DevInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ getName NOTIFY changed)
    Q_PROPERTY(QString address READ getAddress NOTIFY changed)
public:
    typedef enum {
        SRS_UNKNOWN,
        SRC_WIFI,
        SRC_BLUETOOTH
    } src_t;

    explicit DevInfo() = default;
    explicit DevInfo(const QBluetoothDeviceInfo &bt);
    explicit DevInfo(const WifiDeviceItem &wf);
    QString getAddress() const;
    QString getName() const;
    void set(const QBluetoothDeviceInfo &bt);
    void set(const WifiDeviceItem &wf);

signals:
    void changed();

private:
    src_t m_type = SRS_UNKNOWN;
    QBluetoothDeviceInfo m_bt;
    WifiDeviceItem       m_wifi;
};

#endif // DEVINFO_H
