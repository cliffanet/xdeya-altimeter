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

#define FSTR(len, str) if (fh.write( str ) < len) return false
#define FFMT(fmt, ...)  \
        do { \
            snprintf(s, sizeof(s), fmt, ##__VA_ARGS__); \
            FSTR(5, s); \
        } while (0)

/*
bool TrackHnd::saveGPX(QIODevice &fh) const
{
    if (!fh.isOpen() || !fh.isWritable())
        return false;

    FSTR(100,
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
            FSTR(5, isok ? "<trkseg>" : "</trkseg>");
            prevok = isok;
        }
        if (!isok)
            continue;

        FFMT("<trkpt lat=\"%0.6f\" lon=\"%0.6f\">",
             static_cast<double>(p.lat)/10000000,
             static_cast<double>(p.lon)/10000000);

        uint32_t sec = p.tmoffset / 1000;
        uint32_t min = sec / 60;
        sec -= min*60;
        FFMT("<name>%d:%02d, %d m / %0.1f m/s</name>",
             min, sec, p.alt, static_cast<double>(p.altspeed)/100
        );

        FFMT("<desc>Горизонт: %d&deg; / %0.1f m/s", p.heading, static_cast<double>(p.hspeed)/100);
        if (p.altspeed > 0) {
            FFMT(" (кач: %0.1f)", -1.0 * p.hspeed / p.altspeed);
        }
        FFMT("</desc>");

        FFMT("<ele>%d</ele>", p.alt);
        FFMT("<magvar>%d</magvar>", p.heading);
        FFMT("<sat>%d</sat>", p.sat);

        FFMT("</trkpt>");
    }

    if (prevok)
        FSTR(5, "</trkseg>");

    FSTR(10,
            "</trk>"
        "</gpx>"
    );

    return true;
}
*/

#undef FSTR

#define FSTR(len, str) buf.append( str )

void TrackHnd::saveGeoJson(QByteArray &buf) const
{
    FSTR(10,
        "{"
            "\"type\": \"FeatureCollection\","
            "\"features\": ["
    );

    bool prevok = false;
    uint8_t state = 0;
    uint32_t segid = 0;
    char s[1024];
    QByteArray prop;

    for (const auto &p : m_data) {
        bool isok = (p.flags & 0x0001) > 0;
        if (prevok && (state != p.state)) {
            prop.append("]}}");
            FFMT("]}%s},", prop.constData());
            prevok = false;
            state = p.state;
        }
        if (prevok != isok) {
            if (isok) {
                segid++;
                const char *color =
                    (p.state == 's') || (p.state == 't') ? // takeoff
                        "#2e2f30" :
                    p.state == 'f' ? // freefall
                        "#7318bf" :
                    (p.state == 'c') || (p.state == 'l') ? // canopy
                        "#0052ef" :
                        "#fcb615";
                FFMT(
                    "{"
                        "\"type\": \"Feature\","
                        "\"id\": %u,"
                        "\"options\": {\"strokeWidth\": 4, \"strokeColor\": \"%s\"},"
                        "\"hint\": \"asdfasdf\","
                        "\"geometry\": {"
                            "\"type\": \"LineString\","
                            "\"coordinates\": [",
                    segid, color
                );
                prop.clear();
                prop.append(", \"properties\": { \"hintContent\": \"Москва-Берлин\", \"metaDataProperty\": { \"gpxPoints\": [");
                //"\"properties\": { \"hintContent\": \"Москва-Берлин\" },"
            }
            else
                FSTR(4, "]}},");
            prevok = isok;
        }
        if (!isok)
            continue;

        FFMT("[%0.7f,%0.7f],",
             static_cast<double>(p.lat)/10000000,
             static_cast<double>(p.lon)/10000000);

        uint32_t sec = p.tmoffset / 1000;
        uint32_t min = sec / 60;
        snprintf(s, sizeof(s), "{ \"name\": \"%d:%02d\" },",
                 min, sec//, p.alt, static_cast<double>(p.altspeed)/100
                 );
        //prop.append(s);
    }

    if (prevok)
        FSTR(4, "]}},");

    FSTR(2, "]}");
}

#undef FSTR
#undef FFMT
