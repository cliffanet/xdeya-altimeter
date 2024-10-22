/*
    files binary functions
*/

#ifndef _core_filebin_H
#define _core_filebin_H

#include "file.h"


/* ------------------------------------------------------------------------------------------- *
 *  FileBin - работа с бинарными файлами
 * ------------------------------------------------------------------------------------------- */
class FileBin : public FileMy {
    public:
        FileBin() : FileMy() {}
        FileBin(const FileMy &f) : FileMy(f) {}
        FileBin(const char *fname_P, mode_t mode = MODE_READ, bool external = false) :
            FileMy(fname_P, mode, external)
            {}
        
        size_t read(uint8_t *data, uint16_t sz) { return fh.read(data, sz); }
        bool get(uint8_t *data, uint16_t dsz, bool tailnull = true);
        template <typename T>
        bool get(T &data, bool tailnull = true) {
            return get(reinterpret_cast<uint8_t *>(&data), sizeof(T), tailnull);
        }
        
        size_t write(uint8_t *data, uint16_t sz) { return fh.write(data, sz); }
        bool add(const uint8_t *data, uint16_t dsz);
        template <typename T>
        bool add(const T &data) {
            return add(reinterpret_cast<const uint8_t *>(&data), sizeof(T));
        }
};



/* ------------------------------------------------------------------------------------------- *
 *  FileBinNum - работа с бинарными файлами, которые пишуться как лог файлы с суффиксом .N
 * ------------------------------------------------------------------------------------------- */
class FileBinNum : public FileBin {
    public:
        FileBinNum() : FileBin(), m_fname_P(NULL) {}
        FileBinNum(const char *fname_P) : FileBin(), m_fname_P(fname_P) {}
        
        const char *fname_P() const { return m_fname_P; };
        
        bool open(uint8_t n, mode_t mode, bool external);
        virtual bool open(uint8_t n, mode_t mode = MODE_READ) {
            return open(n, mode, false);
        }
        bool open(mode_t mode, bool external) {
            return open(1, mode, external);
        }
        virtual bool open(mode_t mode = MODE_READ) {
            return open(1, mode, false);
        }
        
        size_t count(bool external);
        virtual size_t count() { return count(false); }
        bool renum(bool external);
        virtual bool renum() { return renum(false); }
        bool rotate(uint8_t count, bool external);
        virtual bool rotate(uint8_t count = 0) { return rotate(count, false); }

        bool exists(uint8_t n, bool external);
        virtual bool exists(uint8_t n) { return exists(n, false); }
        bool remove(uint8_t n, bool external);
        virtual bool remove(uint8_t n) { return remove(n, false); }
    
    private:
        const char *m_fname_P;    
};

#endif // _core_filebin_H
