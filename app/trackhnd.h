#ifndef TRACKHND_H
#define TRACKHND_H

#include <QObject>
#include <QList>

#include "nettypes.h"

class TrackHnd : public QObject
{
    Q_OBJECT
public:
    explicit TrackHnd(QObject *parent = nullptr);

    const trkinfo_t & inf() const { return m_inf; }
    void setinf(const trkinfo_t &inf);
    void add(const log_item_t &ti);
    void clear();

    bool mapCenter(log_item_t &ti) const { if (m_ismapcenter) ti = m_mapcenter; return m_ismapcenter; }
    bool mapCenter() const { return m_ismapcenter; }
    bool saveGPX(QIODevice &fh) const;
    void saveGeoJson(QByteArray &fh) const;

signals:
    void onMapCenter(const log_item_t &);

private:
    trkinfo_t m_inf;
    QList<log_item_t> m_data;
    bool m_ismapcenter;
    log_item_t m_mapcenter;
};

#endif // TRACKHND_H
