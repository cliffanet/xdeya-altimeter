#ifndef JMPINFO_H
#define JMPINFO_H

#include <QObject>
#include <QList>
#include <QVariant>

#include "nettypes.h"

class TrkInfo;

class JmpInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int      index       READ index          NOTIFY changed)
    Q_PROPERTY(QString  num         READ getNum         NOTIFY changed)
    Q_PROPERTY(QString  date        READ getDate        NOTIFY changed)
    Q_PROPERTY(QString  timeTakeoff READ getTimeTakeoff NOTIFY changed)
    Q_PROPERTY(QString  dtBeg       READ getDateTimeBeg NOTIFY changed)
    Q_PROPERTY(int      altBeg      READ getAltBeg      NOTIFY changed)
    Q_PROPERTY(QString  timeFF      READ getTimeFF      NOTIFY changed)
    Q_PROPERTY(int      altCnp      READ getAltCnp      NOTIFY changed)
    Q_PROPERTY(QString  timeCnp     READ getTimeCnp     NOTIFY changed)
    Q_PROPERTY(QVariant trklist     READ getTrkList     NOTIFY trkListChanged)
public:
    explicit JmpInfo() = default;
    explicit JmpInfo(const logbook_item_t &jmp, int index = -1);

    const logbook_item_t & jmp() const { return m_jmp; }
    int index() const { return m_index; }
    void setindex(int index);

    QString getNum() const;
    QString getDate() const;
    QString getTimeTakeoff() const;
    QString getDateTimeBeg() const;
    int     getAltBeg() const;
    QString getTimeFF() const;
    int     getAltCnp() const;
    QString getTimeCnp() const;
    QVariant getTrkList() const;

    void set(const logbook_item_t &jmp, int index = -1);
    void set(const JmpInfo &jmp, int index = -1);
    void set(const JmpInfo *jmp, int index = -1);
    void clear();

    void fetchTrkList(const QList<TrkInfo *> &trklist);

signals:
    void changed();
    void trkListChanged();

private:
    logbook_item_t m_jmp;
    int m_index;
    QList<TrkInfo *> m_trklist;
};

#endif // JMPINFO_H
