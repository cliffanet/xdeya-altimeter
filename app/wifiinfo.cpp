#include "wifiinfo.h"

WiFiInfo::WiFiInfo(const char *ssid, const char *pass) :
    m_ssid(ssid),
    m_pass(pass)
{

}

QString WiFiInfo::getPassStar() const
{
    QString star("*");
    return star.repeated(m_pass.length());
}

void WiFiInfo::set(const char *ssid, const char *pass)
{
    m_ssid = ssid != nullptr ? ssid : "";
    m_pass = pass != nullptr ? pass : "";
    emit changed();
}

void WiFiInfo::setSSID(const char *ssid)
{
    m_ssid = ssid != nullptr ? ssid : "";
    emit changed();
}

void WiFiInfo::setSSID(const QString ssid)
{
    m_ssid = ssid;
    emit changed();
}

void WiFiInfo::setPass(const char *pass)
{
    m_pass = pass != nullptr ? pass : "";
    emit changed();
}

void WiFiInfo::setPass(const QString pass)
{
    m_pass = pass;
    emit changed();
}

void WiFiInfo::clear()
{
    m_ssid = "";
    m_pass = "";
    emit changed();
}
