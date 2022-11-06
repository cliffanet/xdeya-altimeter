#ifndef NETPROCESS_H
#define NETPROCESS_H

#include <QObject>
#include "BinProto/nettcpsocket.h"
#include "BinProto/BinProto.h"

class QHostAddress;
class QTcpSocket;

typedef struct __attribute__((__packed__)) tm_s {
    uint16_t year;     // Year                         (1999..2099)
    uint8_t  mon;      // Month                        (1..12)
    uint8_t  day;      // Day of month                 (1..31)
    uint8_t  h;        // Hour of day                  (0..23)
    uint8_t  m;        // Minute of hour               (0..59)
    uint8_t  s;        // Second of minute             (0..59)
    uint8_t  tick;     // tick count (tick = TIME_TICK_INTERVAL ms)

    bool operator== (const struct tm_s & tm) {
        return
            (this == &tm) ||
            (
                (this->year == tm.year) && (this->mon == tm.mon) && (this->day == tm.day) &&
                (this->h == tm.h) && (this->m == tm.m) && (this->s == tm.s)
            );
    };
    bool operator!= (const struct tm_s & tm) { return !(*this == tm); };
} tm_t;

// Один из элементов в длинном логбуке (несколько раз в сек)
typedef struct __attribute__((__aligned__(64), __packed__)) {
    int32_t     tmoffset;   // время от начала измерений        (ms)
    uint16_t    flags;      // флаги: валидность
    uint8_t     state;      // статус высотомера (земля, подъём, падение, под куполом)
    uint8_t     direct;     // направление движения по высоте
    int16_t     alt;        // высота по барометру              (m)
    int16_t     altspeed;   // скорость падения по барометру    (cm/s)
    int32_t     lon;        // Longitude                        (deg * 10^7)
    int32_t     lat;        // Latitude                         (deg * 10^7)
    int32_t     hspeed;     // Ground speed                     (cm/s)
    int16_t     heading;    // направление движения             (deg)
    int16_t     gpsalt;     // высота по NAV (над ур моря)      (m)
    int32_t     vspeed;     // 3D speed                         (cm/s)
    uint32_t    gpsdage;    // gps data age  // тут вместо millis уже используется только 1 байт для хранения, можно пересмотреть формат
    uint8_t     sat;        // количество найденных спутников
    uint8_t     _;
    uint16_t    batval;     // raw-показания напряжения на батарее
    // поля для отладки
    uint32_t    hAcc;       // Horizontal accuracy estimate (mm)
    uint32_t    vAcc;       // Vertical accuracy estimate   (mm)
    uint32_t    sAcc;       // Speed accuracy estimate      (cm/s)
    uint32_t    cAcc;       // Heading accuracy estimate    (deg * 10^5)
    //tm_t        tm;
    uint32_t    millis;     // для отладки времени tmoffset
    uint32_t    msave;
} log_item_t;

typedef struct __attribute__((__packed__)) {
    uint32_t    num;
    uint32_t    key;
    tm_t        tm;
    log_item_t  toff;
    log_item_t  beg;
    log_item_t  cnp;
    log_item_t  end;
} logbook_item_t;

#define LOG_PK              "NnaaiiNNNiiNNCCnNNNNNN"

class NetProcess : public QObject
{
    Q_OBJECT
public:
    typedef enum {
        errNoError,
        errProto,
        errAuth
    } err_t;

    typedef enum {
        wtDisconnected = -1,
        wtUnknown = 0,
        wtInit,
        wtAuth,
        wtLogBookBeg,
        wtLogBook
    } wait_t;

    explicit NetProcess(QObject *parent = nullptr);

    void connectTcp(const QHostAddress &ip, quint16 port);
    void disconnect();
    void resetAll();

    err_t err() const { return m_err; }
    wait_t wait() const { return m_wait; }
    bool connected() const { return m_pro.rcvstate() > BinProto::RCV_DISCONNECTED; }

    bool requestInit();
    bool requestAuth(uint16_t code);
    bool requestLogBook(uint32_t beg = 50, uint32_t count = 50);

    const QList<logbook_item_t> & logbook() const { return m_logbook; }

signals:
    void waitChange(wait_t _wait);
    void rcvInit();
    void rcvAuth(bool isok);
    void rcvData(quint32 pos, quint32 max);
    void rcvLogBook();

private:
    err_t m_err;
    wait_t m_wait;
    BinProto m_pro;
    NetTcpSocket m_nettcp;
    QTcpSocket *tcpClient;
    quint32 m_rcvpos, m_rcvcnt;
    QList<logbook_item_t> m_logbook;

    void tcpConnected();
    void tcpDisconnected();
    void tcpError();

    void setWait(wait_t _wait);
    void rcvProcess();
    void rcvWrong();
};

#endif // NETPROCESS_H
