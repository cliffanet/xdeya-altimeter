#ifndef MODLOGBOOK_H
#define MODLOGBOOK_H

#include <QAbstractTableModel>
#include "nettypes.h"

class ModLogBook : public QAbstractTableModel
{
    Q_OBJECT

public:
    ModLogBook(const QList<logbook_item_t> &_list, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    const QList<logbook_item_t> &m_list;

};

#endif // MODLOGBOOK_H
