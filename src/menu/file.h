/*
    Menu File class
*/

#ifndef _menu_file_H
#define _menu_file_H

#include "base.h"
#include <vector>

typedef struct {
    char name[33];
    size_t sz;
} filei_t;


class MenuFile : public MenuBase {
    public:
        MenuFile();
        void updSize();
        void getStr(menu_dspl_el_t &str, int16_t i);
        
        void btnSmp();
        bool useLng() { return true; } // используется ли длинное нажатие
        void btnLng();
        
    private:
        std::vector<filei_t> fileall;
};

#endif // _menu_file_H
