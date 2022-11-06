#include "modlogbook.h"

#include <QBluetoothAddress>

ModLogBook::ModLogBook(const QList<logbook_item_t> &_list, QObject *parent)
    : QAbstractTableModel(parent),
    m_list(_list)
{

}

int ModLogBook::rowCount(const QModelIndex & /*parent*/) const
{
   return m_list.size();
}

int ModLogBook::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

QVariant ModLogBook::data(const QModelIndex &index, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            {
                auto &d = m_list[index.row()];
                switch (index.column()) {
                    case 0: return QString::number(d.num);
                    case 1: {
                        char s[64];
                        snprintf(s, sizeof(s), "%d.%02d.%04d % 2d:%02d", d.tm.day, d.tm.mon, d.tm.year, d.tm.h, d.tm.m);
                        return QString(s);
                    }
                }
            }
            break;
    }

    return QVariant();
}

Qt::ItemFlags ModLogBook::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) & ~(Qt::ItemIsEditable);
}
