#ifndef MODBTSRCH_H
#define MODBTSRCH_H

#include <QAbstractTableModel>
#include <QBluetoothDeviceInfo>

class ModBtSrch : public QAbstractTableModel
{
    Q_OBJECT
    typedef QList<QBluetoothDeviceInfo> CList;

public:
    ModBtSrch(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void update(const CList &list);

private:
    CList m_list;

};

#endif // MODBTSRCH_H
