//Stupid wraparound include to make sure object file doesn't end up in heatshrink dir

#include <stddef.h> /* for size_t */

#define ICACHE_FLASH_ATTR __attribute__((section(".irom0.text")))	/* I really should include the correct header file */
#define os_memcpy ets_memcpy						/* I really should include the correct header file */

extern void *ets_memcpy(void *dest, const void *src, size_t n);

#include "../lib/heatshrink/heatshrink_encoder.c"
