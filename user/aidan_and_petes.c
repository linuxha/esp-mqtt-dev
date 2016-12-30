/*
 * aidan_and_petes.c
 *
 *  Created on: 17 Jun 2015
 *      Author: Peter
 */
#include "aidan_and_petes.h"


 void IFA iprintf(uint16_t debug_type, char *fmt, ... ){
  char buf[128]; // resulting string limited to 127 chars - change it if you like
  va_list args;
  va_start (args, fmt);
  ets_vsnprintf(buf, sizeof(buf), fmt, args);
  va_end (args);
  if (debug_type & enable_debug_messages) uart0_tx_buffer(buf,os_strlen(buf));

 }


