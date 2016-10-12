#include "vsprintf.h"

int __vsprintf_len(const char *format)
{
	int len = 0;

	if (format == 0){
		return 0;
	}

	while(*format)
	{
		switch(*format)
		{
			case '1':
			case '2':
			case '3':
			case '4':
				len += *format-'0';
				break;
			case 'b':
			case 'c':
				len += (*++format-'0')*10 + (*++format-'0');
				break;
			case 'A':
				len += 6;
				break;
			case 'H':
				len+=2;
				break;
		}
		format++;
	}

	return len;
}

int __vsprintf(u8 *data, const char *format, va_list argptr)
{
	int len, copy_len;
	int word;
	u8 *p = data;

	while(*format)
	{
		switch(*format)
		{
			case '1':
			case '2':
				word = va_arg(argptr, int);
				*data++ = word;
				if (*format=='2'){
					*data++=word>>8;
				}
				break;
			case '3':
			case '4':
				word = va_arg(argptr, int);
				*data++ = word;
				*data++ = word>>8;
				*data++ = word>>16;
				if (*format=='4'){
					*data++=word>>24;
				}
				break;
			case 'b':
				len = (*++format-'0')*10 + (*++format-'0');
				while (len--) {
					word = va_arg(argptr, int);
					*data++ = word;
				}
				break;

			case 'c':
				len = (*++format-'0')*10 + (*++format-'0');
				word = va_arg(argptr, int);
				memcpy(data, (u8 *)word, len);
				data+=len;
				break;
			case 'A':
				word = va_arg(argptr, int);
				memcpy(data, (u8 *)word, 6);
				data+=6;
				break;
			case 'H':
				word = va_arg(argptr, int);
				*data++ = word;
				*data++=word>>8;
				break;
			case 'L':
				copy_len = va_arg(argptr, int);
				break;
			case 'B':
				word = va_arg(argptr, int);
				memcpy(data, (u8 *)word, copy_len);
				data += copy_len;
				break;

			default:
				printf("unknow foramt: %c\n", *format);
				break;
		}
		format++;
	}

	return data-p;
}

