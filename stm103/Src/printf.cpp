#include <stdarg.h>
#include "stm32f1xx_hal.h"
#include "types.h"
#include "main.h"
#include "util.h"

enum
{
    PAD_RIGHT = 1,
    PAD_ZERO = 2
};

static void printchar(char **str, int c)
{
    char *p = *str;
    if(p != null)
    {
        *p++ = c;
        *str = p;
    }
}

static int prints(char **out, const char *string, int width, int pad)
{
	int pc = 0, padchar = ' ';

	if (width > 0) {
		int len = 0;
		const char *ptr;
		for (ptr = string; *ptr; ++ptr) { ++len; }
		if (len >= width) { width = 0;}
		else { width -= len;}
		if (pad & PAD_ZERO) { padchar = '0';}
	}
	if (!(pad & PAD_RIGHT)) {
		for ( ; width > 0; --width) {
			printchar (out, padchar);
			++pc;
		}
	}
	for ( ; *string ; ++string) {
		printchar (out, *string);
		++pc;
	}
	for ( ; width > 0; --width) {
		printchar (out, padchar);
		++pc;
	}

	return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(char **out, int i, int b, int sg, int width, int pad, int letbase)
{
	char print_buf[PRINT_BUF_LEN];
	char *s;
	int t, neg = 0, pc = 0;
	unsigned int u = i;

	if (i == 0) {
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints (out, print_buf, width, pad);
	}

	if (sg && b == 10 && i < 0) {
		neg = 1;
		u = -i;
	}

	s = print_buf + PRINT_BUF_LEN-1;
	*s = '\0';

	while (u) {
		t = u % b;
		if( t >= 10 ) {
			t += letbase - '0' - 10;
        }
		*--s = t + '0';
		u /= b;
	}

	if (neg) {
		if( width && (pad & PAD_ZERO) ) {
			printchar (out, '-');
			++pc;
			--width;
		}
		else {
			*--s = '-';
		}
	}

	return pc + prints (out, s, width, pad);
}

static int print(char **out, char const *format, va_list v)
{
	int width, pad;
	int pc = 0;
	char scr[2];

	for (; *format != 0; ++format) {
		if (*format == '%') {
			++format;
			width = pad = 0;
			if (*format == '\0') { break;}
			if (*format == '%') { goto output;}
			if (*format == '-') {
				++format;
				pad = PAD_RIGHT;
			}
			while (*format == '0') {
				++format;
				pad |= PAD_ZERO;
			}
			for ( ; *format >= '0' && *format <= '9'; ++format) {
				width *= 10;
				width += *format - '0';
			}
			if( *format == 's' ) {
				char *s = va_arg(v, char *);
				pc += prints (out, s?s:"(null)", width, pad);
				continue;
			}
			if( *format == 'd' ) {
				pc += printi (out, va_arg(v, int), 10, 1, width, pad, 'a');
				continue;
			}
			if( *format == 'x' ) {
				pc += printi (out, va_arg(v, int), 16, 0, width, pad, 'a');
				continue;
			}
			if( *format == 'X' ) {
				pc += printi (out, va_arg(v, int), 16, 0, width, pad, 'A');
				continue;
			}
			if( *format == 'u' ) {
				pc += printi (out, va_arg(v, int), 10, 0, width, pad, 'a');
				continue;
			}
			if( *format == 'c' ) {
				/* char are converted to int then pushed on the stack */
				scr[0] = va_arg(v, int);
				scr[1] = '\0';
				pc += prints (out, scr, width, pad);
				continue;
			}
		}
		else {
		output:
			printchar (out, *format);
			++pc;
		}
	}
	**out = '\0';
	return pc;
}

int sprintf(char *out, const char *format, ...)
{
    va_list v;
    va_start(v, format);
	return print(&out, format, v);
}

extern byte uart3_transmit_buffer[];
extern UART_HandleTypeDef uart3;
extern volatile int uart_busy;

// if there's already one going on, it will block until that one is finished
// so if you do a shedload of printf, the light animations will stutter

int printf(char const *format, ...)
{
    va_list v;
    va_start(v, format);
    char buffer[128];
    char *p = buffer;
    int len = print(&p, format, v);
    while(uart_busy) {
    }
    uart_busy = 1;
    memcpy(uart3_transmit_buffer, (byte *)buffer, len);
    HAL_UART_Transmit_IT(&uart3, uart3_transmit_buffer, len);
    return len;
}
