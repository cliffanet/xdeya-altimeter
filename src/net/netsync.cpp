/*
    Network processing for Web-sync
*/

#include "netsync.h"

#include <Arduino.h>

/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса ConfigWebJoin
 * ------------------------------------------------------------------------------------------- */

NetSync::NetSync(Client &_cli)
{
    cli = &_cli;
}

NetSync::~NetSync() {
    if (cli == NULL)
        return;
    cli->stop();
}

bool NetSync::process() {
    if ((cli == NULL) || ended())
        return false;
    
    switch (_state) {
        case NSYNC_INIT:
            cli->stop();
            _state = NSYNC_CONNECTING;
            return true;
        
        case NSYNC_CONNECTING:
            if (!cli->connect("gpstat.dev.cliffa.net", 9971)) {
                strcpy_P(_error, PSTR("server can't connect"));
                _state = NSYNC_ERROR;
                return false;
            }
            Serial.println("server connected");
            
            {
                uint8_t d[8] = { '%', 0x01, 0, 4 };
                cli->write(d, sizeof(d));
            }
            
            _state = NSYNC_PROCESS;
            return true;
        
        case NSYNC_PROCESS:
            while (cli->available() > 0) {
                break;
            }
            cli->flush();
            cli->stop();
            _state = NSYNC_END;
            Serial.println("server finished");
            return false;
        
    }
    
}
