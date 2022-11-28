#ifndef TRKINFO_H
#define TRKINFO_H

#include <QObject>

#include "nettypes.h"

class TrkInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int     index  READ index     NOTIFY changed)
    Q_PROPERTY(QString jmpnum READ getJmpNum NOTIFY changed)
    Q_PROPERTY(QString dtBeg READ getDateTimeBeg NOTIFY changed)
    Q_PROPERTY(trklist_item_t trk READ trk NOTIFY changed)
public:
    explicit TrkInfo() = default;
    explicit TrkInfo(const trklist_item_t &trk, int index = -1);

    const trklist_item_t & trk() const { return m_trk; }
    int index() const { return m_index; }
    void setindex(int index);

    QString getJmpNum() const;
    QString getDateTimeBeg() const;

    void set(const trklist_item_t &trk, int index = -1);

signals:
    void changed();

private:
    trklist_item_t m_trk;
    int m_index;
};

#endif // TRKINFO_H
