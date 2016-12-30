/*
 * aidan_and_petes.h
 *
 *  Created on: 16 Mar 2015
 *      Author: Aidan
 */

#ifndef USER_AIDAN_AND_PETES_H_
#define USER_AIDAN_AND_PETES_H_

#include "espmissingincludes.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include <stdarg.h>

// Aidans Additions

#include "iodefs.h"

#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "auth.h"

#ifndef IFA
#define IFA		ICACHE_FLASH_ATTR
#endif

#ifndef IRA
#define IRA     ICACHE_RODATA_ATTR
#endif

MQTT_Client mqttClient; // Main MQTT client

void iprintf(uint16_t debug_type, char *fmt, ... );

//static uint8_t wsBitMask; // unused
uint8_t indic_buf[3];

#endif /* USER_AIDAN_AND_PETES_H_ */
