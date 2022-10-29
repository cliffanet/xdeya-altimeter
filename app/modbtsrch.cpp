#include "modbtsrch.h"

#include <QBluetoothAddress>

ModBtSrch::ModBtSrch(QObject *parent)
    : QAbstractTableModel(parent)
{

}

int ModBtSrch::rowCount(const QModelIndex & /*parent*/) const
{
   return m_list.size();
}

int ModBtSrch::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant ModBtSrch::data(const QModelIndex &index, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            {
                auto &p = m_list[index.row()];
                switch (index.column()) {
                    case 0: return p.name();
                }
            }
            break;
    }

    return QVariant();
}

Qt::ItemFlags ModBtSrch::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) & ~(Qt::ItemIsEditable);
}

void ModBtSrch::update(const CList &list)
{
    m_list.clear();
    for (auto &item: list)
        if (
                item.isValid() &&
                !item.address().isNull() &&
                (item.name() != tr(""))
                //(item.address().toString() != tr("00:00:00:00:00:00") )
            )
            m_list.push_back(item);

    emit layoutChanged();
}
