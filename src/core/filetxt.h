/*
    files text functions
*/

#ifndef _core_filetxt_H
#define _core_filetxt_H

#include "file.h"

/* ------------------------------------------------------------------------------------------- *
 *  FileTxt - работа с текстовыми файлами
 * ------------------------------------------------------------------------------------------- */
class FileTxt : public FileMy {
    public:
        FileTxt() : FileMy() {}
        FileTxt(const FileMy &f) : FileMy(f) {}
        FileTxt(const char *fname_P, mode_t mode = MODE_READ, bool external = false) :
            FileMy(fname_P, mode, external)
            {}
        
        // read_param - читает текст до появления пробельного символа
        // все пробелы будут пропущены, но если наткнёмся на "перевод строки", он не будет пропущен
        int read_param(char *str, size_t sz);
        // read_line - читает строку до обнаружения "перевода строки",
        // при этом сам символ "перевода строки" будет пропущен,
        // чтобы следующий вызов read_param/read_line начался уже со следующей строки
        int read_line(char *str, size_t sz);
        
        // Таким образом, если нам надо прочитать "параметр значение" через пробел,
        // вызываем поочерёдно read_param и потом read_line - и получим гарантированно чтение одного и другого
        // без случайного пропуска строки, если перед этим была строка только с "параметром", но без "значения"
        
        // Вызов read_param/read_line при str=NULL или sz=0 - выполнит такое же чтение, но не сохранит в строку
        
        // find_param - делает read_param/read_line до тех пор, пока не наткнётся на искомый параметр,
        // после чего можно обращаться к read_line, чтобы прочесть значение этого параметра
        bool find_param(const char *name_P);
        
        bool print_line(const char *str);
        bool print_param(const char *name_P);
        bool print_param(const char *name_P, const char *str);
        
        uint32_t chksum();
};

#endif // _core_filetxt_H
