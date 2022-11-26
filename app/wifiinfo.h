#ifndef WIFIINFO_H
#define WIFIINFO_H

#include <QObject>

class WiFiInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString  ssid        READ getSSID        NOTIFY changed)
    Q_PROPERTY(QString  pass        READ getPass        NOTIFY changed)
    Q_PROPERTY(QString  passstar    READ getPassStar    NOTIFY changed)
public:
    explicit WiFiInfo() = default;
    explicit WiFiInfo(const char *ssid, const char *pass);

    const QString &getSSID() const { return m_ssid; };
    const QString &getPass() const { return m_pass; };
    QString getPassStar() const;

    void set(const char *ssid, const char *pass);
    void setSSID(const char *ssid);
    void setSSID(const QString ssid);
    void setPass(const char *pass);
    void setPass(const QString pass);
    void clear();

signals:
    void changed();

private:
    QString m_ssid, m_pass;
};

#endif // WIFIINFO_H
