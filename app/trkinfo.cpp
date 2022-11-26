#include "trkinfo.h"

#include <QDateTime>
#include <QDate>
#include <QTime>

static QString dt2format(const tm_t &tm, QString format = "d.MM.yyyy hh:mm") {
    QDateTime dt(QDate(tm.year, tm.mon, tm.day), QTime(tm.h, tm.m, tm.s));
    return dt.toString(format);
}

TrkInfo::TrkInfo(const trklist_item_t &trk) :
    m_trk(trk)
{
}

QString TrkInfo::getJmpNum() const
{
    return QString::number(m_trk.jmpnum);
}

QString TrkInfo::getDateTimeBeg() const
{
    return dt2format(m_trk.tmbeg);
}

void TrkInfo::set(const trklist_item_t &trk)
{
    m_trk = trk;
    emit changed();
}
