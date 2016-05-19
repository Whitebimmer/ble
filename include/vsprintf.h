#ifndef VSPRINTF_H
#define VSPRINTF_H


#include "typedef.h"
#include "stdarg.h"



int __vsprintf_len(const char *format);


int __vsprintf(u8 *ret, const char *format, va_list argptr);









#endif

