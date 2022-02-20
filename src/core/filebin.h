/*
    files binary functions
*/

#ifndef _core_filebin_H
#define _core_filebin_H

#include "file.h"

/* ------------------------------------------------------------------------------------------- *
 *  Прямая работа с записью в файле
 *
 *  Вынесем в отдельные функции, т.к. этот же алгоритм используют конфиги.
 *  Но там используется разовое чтение/запись (только одна запись на файл),
 *  поэтому использование FileMy может быть расточительным с т.з. 
 *  поиска свободного слота в fhall[].
 * ------------------------------------------------------------------------------------------- */
namespace fs { class File; }
bool recBinRead(fs::File &fh, uint8_t *data, uint16_t dsz, bool tailnull = true);

template <typename T>
bool recBinRead(fs::File &fh, T &data, bool tailnull = true) {
    return recBinRead(fh, reinterpret_cast<uint8_t *>(&data), sizeof(T), tailnull);
}

bool recBinWrite(fs::File &fh, const uint8_t *data, uint16_t dsz);

template <typename T>
bool recBinWrite(fs::File &fh, const T &data) {
    return recBinWrite(fh, reinterpret_cast<const uint8_t *>(&data), sizeof(T));
}

#endif // _core_filebin_H
