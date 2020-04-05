/*
    Cfg for Web-join
*/

#include "webjoin.h"

/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса ConfigWebJoin
 * ------------------------------------------------------------------------------------------- */

ConfigWebJoin::ConfigWebJoin() :
    Config(PSTR(CFG_WEBJOIN_NAME), CFG_WEBJOIN_VER)
{
    
}

ConfigWebJoin::ConfigWebJoin(uint32_t authid, uint32_t secnum) :
    Config(PSTR(CFG_WEBJOIN_NAME), CFG_WEBJOIN_VER)
{
    data.authid = authid;
    data.secnum = secnum;
}

