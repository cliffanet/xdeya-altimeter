/*
    Network processing for Web-sync
*/

#ifndef _net_sync_H
#define _net_sync_H

#include <Client.h>

typedef enum {
    NSYNC_INIT = 0,
    NSYNC_CONNECTING,
    NSYNC_JOIN,
    NSYNC_PROCESS,
    NSYNC_END,
    NSYNC_ERROR = 255
} net_sync_state_t;

class NetSync {
    public:
        NetSync(Client &_cli);
        ~NetSync();
        
        bool process();
        
        net_sync_state_t state() const { return _state; }
        bool ended() const { return (_state == NSYNC_END) || (_state == NSYNC_ERROR); }
        bool error() const { return _state == NSYNC_ERROR; }
        const char *errstr() const { return _error; }
        
    private:
        net_sync_state_t _state = NSYNC_INIT;
        char _error[25] = { 0 };
        Client *cli = NULL;
};

#endif // _net_sync_H
