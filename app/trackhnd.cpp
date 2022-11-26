#include "trackhnd.h"

#include <QIODevice>
#include <QByteArray>

TrackHnd::TrackHnd(QObject *parent)
    : QObject{parent},
    m_inf({ }),
    m_ismapcenter(false),
    m_mapcenter({ })
{

}

void TrackHnd::setinf(const trkinfo_t &inf)
{
    m_inf = inf;
}

void TrackHnd::add(const log_item_t &ti)
{
    m_data.push_back(ti);
    if (!m_ismapcenter && ((ti.flags & 0x0001) > 0)) {
        m_ismapcenter = true;
        m_mapcenter = ti;
        emit onMapCenter(ti);
    }
}

void TrackHnd::clear()
{
    m_data.clear();
    m_ismapcenter = false;
    memset(&m_inf, 0, sizeof(m_inf));
    memset(&m_mapcenter, 0, sizeof(m_mapcenter));
}

#define FMT(s,fmt, ...) snprintf(s, sizeof(s), fmt, ##__VA_ARGS__)
#define WRT(len, str)  if (fh.write( str ) < len) return false
#define FMTWR(fmt, ...)  \
        do { \
            int len = FMT(s, fmt, ##__VA_ARGS__); \
            WRT(len, s); \
        } while (0)

/*
bool TrackHnd::saveGPX(QIODevice &fh) const
{
    if (!fh.isOpen() || !fh.isWritable())
        return false;

    WRT(100,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<gpx version=\"1.1\" creator=\"XdeYa\" xmlns=\"http://www.topografix.com/GPX/1/1\">"
            "<metadata>"
                "<name><![CDATA[Без названия]]></name>"
                "<desc/>"
                "<time>2017-10-18T12:19:23.353Z</time>"
            "</metadata>"
            "<trk>"
                    "<name>трек</name>"
    );

    bool prevok = false;
    char s[1024];

    for (const auto &p : m_data) {
        bool isok = (p.flags & 0x0001) > 0;
        if (prevok != isok) {
            WRT(5, isok ? "<trkseg>" : "</trkseg>");
            prevok = isok;
        }
        if (!isok)
            continue;

        FMTWR("<trkpt lat=\"%0.6f\" lon=\"%0.6f\">",
             static_cast<double>(p.lat)/10000000,
             static_cast<double>(p.lon)/10000000);

        uint32_t sec = p.tmoffset / 1000;
        uint32_t min = sec / 60;
        sec -= min*60;
        FMTWR("<name>%d:%02d, %d m / %0.1f m/s</name>",
             min, sec, p.alt, static_cast<double>(p.altspeed)/100
        );

        FMTWR("<desc>Горизонт: %d&deg; / %0.1f m/s", p.heading, static_cast<double>(p.hspeed)/100);
        if (p.altspeed > 0) {
            FMTWR(" (кач: %0.1f)", -1.0 * p.hspeed / p.altspeed);
        }
        FMTWR("</desc>");

        FMTWR("<ele>%d</ele>", p.alt);
        FMTWR("<magvar>%d</magvar>", p.heading);
        FMTWR("<sat>%d</sat>", p.sat);

        FMTWR("</trkpt>");
    }

    if (prevok)
        WRT(5, "</trkseg>");

    WRT(10,
            "</trk>"
        "</gpx>"
    );

    return true;
}
*/

#undef WRT

#define WRT(len, str) buf.append( str )

void _geoJsonPoint(const QList<log_item_t>::const_iterator &pp, QByteArray &crd/*, QByteArray &gpx*/) {
    char s[1024];
    snprintf(s, sizeof(s),
             "[%0.7f,%0.7f],",
             static_cast<double>(pp->lat)/10000000,
             static_cast<double>(pp->lon)/10000000);
    crd.append(s);
}

void TrackHnd::saveGeoJson(QByteArray &buf) const
{
    WRT(10,
        "{"
            "\"type\": \"FeatureCollection\","
            "\"features\": ["
    );

    auto pp = m_data.begin();
    auto prv = m_data.begin();
    uint32_t segid = 0;
    char s[1024], time[100], vert[512], horz[512];

    while (pp != m_data.end()) {
        while (// пропустим все точки без спутников
                (pp != m_data.end()) &&
                ((pp->flags & 0x0001) == 0)
              ) {
              pp++;
              prv = pp;
        }
        if (pp == m_data.end())
            break;

        QByteArray crd/*, gpx*/;
        segid++;

        // Предыдущая точка, c неё будем всегда начинать список координат
        _geoJsonPoint(prv, crd/*, gpx*/);

        // очередная партия точек
        auto frst = pp;
        for (
                int n=1;
                (n<=5) && (pp != m_data.end()) && (pp->state == frst->state) && ((pp->flags & 0x0001) > 0);
                pp++, n++
            ) {
            _geoJsonPoint(pp, crd/*, gpx*/);
            prv = pp;
        }

        const char *color =
            (prv->state == 's') || (prv->state == 't') ? // takeoff
                "#2e2f30" :
            prv->state == 'f' ? // freefall
                "#7318bf" :
            (prv->state == 'c') || (prv->state == 'l') ? // canopy
                "#0052ef" :
                "#fcb615";


        uint32_t sec = prv->tmoffset / 1000;
        uint32_t min = sec / 60;
        sec -= min*60;
        FMT(time, "%d:%02d",  min, sec);

        FMT(vert, "%d m (%0.1f m/s)", prv->alt, static_cast<double>(prv->altspeed)/100);

        int l = FMT(horz, "%d&deg; (%0.1f m/s)", prv->heading, static_cast<double>(prv->hspeed)/100);
        if (prv->altspeed < 10)
            snprintf(horz+l, sizeof(horz)-l,
                " [кач: %0.1f]", -1.0 * prv->hspeed / prv->altspeed
            );

        FMTWR(
            "{"
                "\"type\": \"Feature\","
                "\"id\": %u,"
                "\"options\": {\"strokeWidth\": 4, \"strokeColor\": \"%s\"},"
                "\"geometry\": {"
                    "\"type\": \"LineString\","
                    "\"coordinates\": [",
             segid, color
        );
        buf.append(crd);
        FMTWR(
                    "]"
                "},"
                "\"properties\": {"
                    "\"balloonContentHeader\": \"%s\","
                    "\"balloonContentBody\": \"Вертикаль: %s<br />Горизонт: %s\","
                    "\"hintContent\": \"%s<br/>Вер: %s<br/>Гор: %s\",",
                    //"\"metaDataProperty\": {"
                    //    "\"gpxPoints\": ["
            time, vert, horz, time, vert, horz
        );
        //buf.append(gpx);
        WRT(4,
                    //    "]"
                    //"}"
                "}"
            "},"
        );
    }

    WRT(2, "]}");
}

#undef WRT
#undef FMTWR
#undef FMT
