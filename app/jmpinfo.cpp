#include "jmpinfo.h"
#include "trkinfo.h"

#include <QDateTime>
#include <QDate>
#include <QTime>

static QString sec2time(uint sec) {
    uint min = sec / 60;
    sec -= min*60;
    uint hour = min / 60;
    min -= hour*60;

    char s[64];
    snprintf(s, sizeof(s), "%d:%02d:%02d", hour, min, sec);
    return QString(s);
}

static QString dt2format(const tm_t &tm, QString format = "d.MM.yyyy hh:mm") {
    QDateTime dt(QDate(tm.year, tm.mon, tm.day), QTime(tm.h, tm.m, tm.s));
    return dt.toString(format);
}

JmpInfo::JmpInfo(const logbook_item_t &jmp, int index) :
    m_jmp(jmp),
    m_index(index)
{
}

QString JmpInfo::getNum() const
{
    return QString::number(m_jmp.num);
}

QString JmpInfo::getDate() const
{
    char s[64];
    snprintf(s, sizeof(s), "%d.%02d.%04d % 2d:%02d", m_jmp.tm.day, m_jmp.tm.mon, m_jmp.tm.year, m_jmp.tm.h, m_jmp.tm.m);
    return QString(s);
}

QString JmpInfo::getTimeTakeoff() const
{
    return sec2time(m_jmp.toff.tmoffset / 1000);
}

QString JmpInfo::getDateTimeBeg() const
{
    return dt2format(m_jmp.tm);
}

int JmpInfo::getAltBeg() const
{
    return m_jmp.beg.alt;
}

QString JmpInfo::getTimeFF() const
{
    return sec2time((m_jmp.cnp.tmoffset - m_jmp.beg.tmoffset) / 1000);
}

int JmpInfo::getAltCnp() const
{
    return m_jmp.cnp.alt;
}

QString JmpInfo::getTimeCnp() const
{
    return sec2time((m_jmp.end.tmoffset - m_jmp.cnp.tmoffset) / 1000);
}

QVariant JmpInfo::getTrkList() const
{
    return QVariant::fromValue(m_trklist);
}

void JmpInfo::set(const logbook_item_t &jmp, int index)
{
    m_index = index;
    m_jmp = jmp;
    emit changed();
}

void JmpInfo::set(const JmpInfo &jmp, int index)
{
    set(jmp.m_jmp, index >= 0 ? index : jmp.m_index);
}

void JmpInfo::set(const JmpInfo *jmp, int index)
{
    if (jmp == nullptr)
        clear();
    else
        set(*jmp, index);
}

void JmpInfo::clear()
{
    m_index = -1;
    set({});
    m_trklist.clear();
    emit trkListChanged();
}

void JmpInfo::fetchTrkList(const QList<trklist_item_t> &trklist)
{
    m_trklist.clear();
    for (auto &trk: trklist) {
        if (trk.jmpnum != m_jmp.num)
            continue;
        m_trklist.push_back(new TrkInfo(trk));
    }

    emit trkListChanged();
}
