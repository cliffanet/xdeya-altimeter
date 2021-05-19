/*
    common
*/

#ifndef _def_H
#define _def_H

#define STRINGIFY(x)        #x
#define TOSTRING(x)         STRINGIFY(x)
#define PPCAT_NX(A, B)      A ## B

#ifndef HWVER
#define HWVER               3
#endif

#if defined(FWVER_DEV) && !defined(FWVER_DEBUG)
#define FWVER_DEBUG
#endif

/* FW version name */
#if defined(FWVER_NUM1) && defined(FWVER_NUM2) && defined(FWVER_NUM3)

#define FWVER_NUM           FWVER_NUM1.FWVER_NUM2.FWVER_NUM3

#ifdef FWVER_DEV
#define FWVER_TYPE_NAME     ".dev"
#define FWVER_TYPE_DSPL     "dev"
#define FWVER_TYPE_CODE     'D'
#elif defined(FWVER_DEBUG)
#define FWVER_TYPE_NAME     ".debug"
#define FWVER_TYPE_DSPL     "debug"
#define FWVER_TYPE_CODE     'd'
#else
#define FWVER_TYPE_NAME     ""
#define FWVER_TYPE_DSPL     "prod"
#define FWVER_TYPE_CODE     'p'
#endif

#define FWVER_NAME          TOSTRING(FWVER_NUM) FWVER_TYPE_NAME ".hw" TOSTRING(HWVER)
#define FWVER_FILENAME      "gpsadv.v" FWVER_NAME

#endif

#if HWVER < 3
#define USE4BUTTON          1
#endif

#endif // _def_H
