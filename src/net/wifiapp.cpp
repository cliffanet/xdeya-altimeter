/*
    WiFi for application functions
*/

#include "wifiapp.h"
#include "../log.h"
#include "../core/workerloc.h"
#include "../core/worker.h"
#include "wifi.h"
#include "../view/menu.h" // MENUSZ_VAL

#include <string.h>

/* ------------------------------------------------------------------------------------------- *
 *  Инициализация
 * ------------------------------------------------------------------------------------------- */

static const char  *m_ssid = NULL, *m_pass = NULL;

void wifiCliBegin(const char *ssid, const char *pass) {
    if (m_ssid != NULL) {
        delete m_ssid;
        m_ssid = NULL;
    }
    if (m_pass != NULL) {
        delete m_pass;
        m_pass = NULL;
    }
    
    m_ssid = strdup(ssid);
    m_pass = pass != NULL ? strdup(pass) : NULL;
    
    if (!wifiStart())
        return;
    
    CONSOLE("wifi to: %s; pass: %s", m_ssid, m_pass == NULL ? "-no-" : m_pass);
    if (!wifiConnect(m_ssid, m_pass))
        return;
    CONSOLE("begin");
}

void wifiCliNet(char *ssid) {
    if (m_ssid == NULL)
        *ssid = '\0';
    else {
        strncpy(ssid, m_ssid, MENUSZ_VAL);
        ssid[MENUSZ_VAL-1] = '\0';
    }
}
