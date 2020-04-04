/*
    Cfg for Web-join
*/

#include "webjoin.h"

ConfigWebJoin webjoin;

/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса ConfigWebJoin
 * ------------------------------------------------------------------------------------------- */

ConfigWebJoin::ConfigWebJoin() :
    Config(PSTR(CFG_WEBJOIN_NAME), CFG_WEBJOIN_VER)
{
    
}

