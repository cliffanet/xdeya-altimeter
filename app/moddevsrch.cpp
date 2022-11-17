#include "moddevsrch.h"

static WifiDeviceItem wifiNull = {};

ModDevSrch::ModDevSrch(QObject *parent)
    : QAbstractTableModel(parent)
{

}

int ModDevSrch::rowCount(const QModelIndex & /*parent*/) const
{
   return m_list.size();
}

int ModDevSrch::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant ModDevSrch::data(const QModelIndex &index, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            {
                auto &d = m_list[index.row()];
                switch (index.column()) {
                    case 0: return d.name;
                }
            }
            break;
    }

    return QVariant();
}

Qt::ItemFlags ModDevSrch::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) & ~(Qt::ItemIsEditable);
}

void ModDevSrch::clear()
{
    m_list.clear();
    emit layoutChanged();
}

void ModDevSrch::add(const CItem &item)
{
    m_list.push_back(item);
    emit layoutChanged();
}

void ModDevSrch::addWifi(const WifiDeviceItem &dev)
{
    CItem item = { SRC_WIFI };
    item.name = dev.name;
    item.wifi = dev;
    add(item);
}

QString ModDevSrch::name(qsizetype i) const
{
    return
        (i >= 0) && (i < m_list.count()) ?
            m_list[i].name :
            QString();
}

ModDevSrch::src_t ModDevSrch::src(qsizetype i) const
{
    return
        (i >= 0) && (i < m_list.count()) ?
            m_list[i].src :
            SRS_UNKNOWN;
}

const WifiDeviceItem &ModDevSrch::wifi(qsizetype i) const
{
    return
        (i >= 0) && (i < m_list.count()) ?
            m_list[i].wifi :
            wifiNull;
}
