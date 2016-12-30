#ifndef ESPMISSINGINCLUDES_H
#define ESPMISSINGINCLUDES_H

#include <stdint.h>
//#include <ets_sys.h> // moved down (grrr)

#ifndef IFA
#define IFA		ICACHE_FLASH_ATTR
#endif

#ifndef IRA
#define IRA     ICACHE_RODATA_ATTR
#endif

// ncherry@linuxha.com
#if !defined(uint8)
#if !defined(__uint8_t)
typedef unsigned char       uint8;
typedef unsigned char       u8;
typedef signed char         sint8;
typedef signed char         int8;
typedef signed char         s8;
typedef unsigned short      uint16;
typedef unsigned short      u16;
typedef signed short        sint16;
typedef signed short        s16;
typedef unsigned int        uint32;
typedef unsigned int        u_int;
typedef unsigned int        u32;
typedef signed int          sint32;
typedef signed int          s32;
typedef int                 int32;
typedef signed long long    sint64;
typedef unsigned long long  uint64;
typedef unsigned long long  u64;
typedef float               real32;
typedef double              real64;

typedef uint8               uint8_t;
typedef uint16              uint16_t;
typedef uint32              uint32_t;
#else
typedef __uint8_t       uint8;
typedef __uint16_t      uint16;
typedef __uint32_t      uint32;
typedef __uint8_t       uint8_t;
typedef __uint16_t      uint16_t;
typedef __uint32_t      uint32_t;
#endif
#endif

#include <ets_sys.h>

//Missing function prototypes in include folders. Gcc will warn on these if we don't define 'em anywhere.
//MOST OF THESE ARE GUESSED! but they seem to swork and shut up the compiler.
typedef struct espconn espconn;

int atoi(const char *nptr);
void ets_install_putc1(void *routine);
void ets_isr_attach(int intr, void *handler, void *arg);
void ets_isr_mask(unsigned intr);
void ets_isr_unmask(unsigned intr);
int ets_memcmp(const void *s1, const void *s2, size_t n);
void *ets_memcpy(void *dest, const void *src, size_t n);
void *ets_memset(void *s, int c, size_t n);
int ets_sprintf(char *str, const char *format, ...)  __attribute__ ((format (printf, 2, 3)));
int ets_str2macaddr(void *, void *);
int ets_strcmp(const char *s1, const char *s2);
char *ets_strcpy(char *dest, const char *src);
size_t ets_strlen(const char *s);
int ets_strncmp(const char *s1, const char *s2, int len);
char *ets_strncpy(char *dest, const char *src, size_t n);
char *ets_strstr(const char *haystack, const char *needle);
void ets_timer_arm_new(ETSTimer *a, int b, int c, int isMstimer);
void ets_timer_disarm(ETSTimer *a);
void ets_timer_setfn(ETSTimer *t, ETSTimerFunc *fn, void *parg);
void ets_update_cpu_frequency(int freqmhz);
int os_printf(const char *format, ...)  __attribute__ ((format (printf, 1, 2)));
int os_snprintf(char *str, size_t size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));

//void pvPortFree(void *ptr);
//void *pvPortMalloc(size_t xWantedSize);
//void *pvPortZalloc(size_t);
// void vPortFree(void *ptr);
// void *vPortMalloc(size_t xWantedSize);

// memory allocation functions are "different" due to memory debugging functionality
// added in SDK 1.4.0
void vPortFree(void *ptr, char * file, int line);
void *pvPortMalloc(size_t xWantedSize, char * file, int line);
void *pvPortZalloc(size_t, char * file, int line);
void *vPortMalloc(size_t xWantedSize);
void pvPortFree(void *ptr);

void uart_div_modify(int no, unsigned int freq);


uint8 wifi_get_opmode(void);
uint32 system_get_time();
//int os_random();
int rand(void);
void ets_bzero(void *s, size_t n);
#endif
