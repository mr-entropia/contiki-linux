/*
 * Minimal compatibility shim for the AVR <avr/pgmspace.h> header
 * used by the Contiki 1.2 sources.
 *
 * On AVR, prog_char/prog_uchar refer to data stored in program
 * (flash) memory and must be read back with memcpy_P() etc.
 *
 * On the Linux port all data lives in ordinary RAM, so prog_char is
 * just const data and the read functions reduce to their normal
 * libc counterparts.
 */
#ifndef __PGMSPACE_H__
#define __PGMSPACE_H__

#include <string.h>

typedef unsigned char prog_char;
typedef unsigned char prog_uchar;
typedef unsigned int prog_int;

#define PROGMEM
#define PGM_P   const char *
#define PGM_VOID_P const void *

#define memcpy_P(dest, src, len) memcpy((dest), (src), (len))
#define strcpy_P(dest, src) strcpy((dest), (src))
#define strncpy_P(dest, src, n) strncpy((dest), (src), (n))
#define strcasecmp_P(a, b) strcasecmp((a), (b))
#define strncasecmp_P(a, b, n) strncasecmp((a), (b), (n))
#define strcmp_P(a, b) strcmp((a), (b))
#define strstr_P(a, b) strstr((a), (b))
#define strlen_P(a) strlen((a))
#define strchr_P(a, c) strchr((a), (c))
#define sprintf_P(s, f, args...) sprintf((s), (f), ##args)
#define snprintf_P(s, n, f, args...) snprintf((s), (n), (f), ##args)

#endif /* __PGMSPACE_H__ */