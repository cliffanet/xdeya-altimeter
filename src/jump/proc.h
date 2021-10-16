/*
    Altimeter
*/

#ifndef _jump_proc_H
#define _jump_proc_H

#include <stdint.h> 
#include <stdlib.h>

#include "../../def.h"
#include "altcalc.h"
#include "../clock.h" // поле tm в log_item_t

// Шаг отображения высоты
#define ALT_STEP            5
// Порог перескока к следующему шагу
#define ALT_STEP_ROUND      3

// Интервал обнуления высоты (ms)
#define ALT_AUTOGND_INTERVAL    600000

// имя файла для хранения простых логов
#define JMPLOG_SIMPLE_NAME          "logsimple"
// Сколько прыжков в одном файле
#define JMPLOG_SIMPLE_ITEM_COUNT    50
// Сколько файлов прыгов максимум
#define JMPLOG_SIMPLE_FILE_COUNT    5

// хранимый буфер для заранее сохранённых log_item_t
#define JMP_PRELOG_SIZE         160

// Порог скорости, при которой считаем, что начался прыжок
#define JMP_SPEED_MIN           15
// Время (кол-во тиков), которое должна удерживаться скорость JMP_SPEED_BEG
#define JMP_SPEED_COUNT         30
// Порог скорости, при котором произойдёт отмена прыжка, если не закончилось время JMP_SPEED_COUNT
#define JMP_SPEED_CANCEL        10
// Время (кол-во тиков), которое надо прибавить к моменту при переходе через JMP_SPEED_MIN
// т.е. примерное время для разгона до скорости JMP_SPEED_MIN
#define JMP_SPEED_PREFIX        30
// Скорость, при котором будет переход из FF в CNP
#define JMP_SPEED_CANOPY        12

// Один из элементов в длинном логбуке (несколько раз в сек)
typedef struct __attribute__((__aligned__(64), __packed__)) {
    int32_t     tmoffset;   // время от начала измерений        (ms)
    uint16_t    flags;      // флаги: валидность
    uint8_t     state;      // статус высотомера (земля, подъём, падение, под куполом)
    uint8_t     direct;     // направление движения по высоте
    int16_t     alt;        // высота по барометру              (m)
    int16_t     altspeed;   // скорость падения по барометру    (cm/s)
    uint32_t    lon;        // Longitude                        (deg * 10^7)
    uint32_t    lat;        // Latitude                         (deg * 10^7)
    uint32_t    hspeed;     // Ground speed                     (cm/s)
    int16_t     heading;    // направление движения             (deg)
    int16_t     gpsalt;     // высота по GPS (над ур моря)      (m)
    uint32_t    vspeed;     // 3D speed                         (cm/s)
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

#define LI_FLAG_GPS_VALID   0x0001
#define LI_FLAG_GPS_VLOC    0x0002
#define LI_FLAG_GPS_VVERT   0x0004
#define LI_FLAG_GPS_VSPEED  0x0008
#define LI_FLAG_GPS_VHEAD   0x0010
#define LI_FLAG_GPS_VTIME   0x0020
#define LI_FLAG_JMPBEG      0x0200
#define LI_FLAG_JMPCNP      0x0400
#define LI_FLAG_JMPEND      0x0800
#define LI_FLAG_JMPDECISS   0x1000
#define LI_FLAG_BTN_UP      0x2000
#define LI_FLAG_BTN_SEL     0x4000
#define LI_FLAG_BTN_DOWN    0x8000

// Один прыг в простом логбуке, где запоминаются только данные на начало, середину и самое окончание прыга
typedef struct __attribute__((__packed__)) {
    uint32_t    num;
    uint32_t    key;
    tm_t        tm;
    log_item_t  toff;
    log_item_t  beg;
    log_item_t  cnp;
    log_item_t  end;
} log_jmp_t;


AltCalc & altCalc();
const log_item_t &jmpPreLog(uint16_t old = 0);
uint32_t jmpPreLogInterval(uint16_t old = 0);
uint16_t jmpPreLogFirst(log_item_t *li = NULL);
bool jmpPreLogNext(uint16_t &cursor, log_item_t *li = NULL);

uint8_t jmpState();

bool jmpTakeoffCheck();

void jmpInit();
void jmpProcess();
void jmpReset();

#endif // _jump_proc_H
