/* main.c
 *
 * Copyright (c) 2014-2015, Aidan Ruff (aidan@ruffs.org) & Peter Scargill (pete@scargill.net)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * * Neither the name of Redis nor the names of its contributors may be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


void user_rf_pre_init(void)
{
}

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


// Aidans Additions

#include "iodefs.h"

#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "auth.h"

#include "aidan_and_petes.h"

//int enable_debug_messages = INFO | DEBUG | RESPONSE;
int enable_debug_messages = INFO | RESPONSE;

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
#if 1
uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void);
#else
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}
#endif

static void IFA wifi_station_scan_done(void *arg, STATUS status) {
	uint8 ssid[33];
	char tBuf[84];
	char dBuf[84];

	os_sprintf(tBuf, "%s/fromesp/scan", sysCfg.base);
	if (status == OK) {
		struct bss_info *bss_link = (struct bss_info *)arg;

		while (bss_link != NULL) {
			os_memset(ssid, 0, 33);
			if (os_strlen(bss_link->ssid) <= 32) {
				os_memcpy(ssid, bss_link->ssid, os_strlen(bss_link->ssid));
			} else {
				os_memcpy(ssid, bss_link->ssid, 32);
			}
			os_sprintf(dBuf, "WiFi Scan: (%d,\"%s\",%d)\r\n", bss_link->authmode, ssid, bss_link->rssi);
			iprintf(RESPONSE, dBuf);
			os_sprintf(dBuf, "{\"mode\":%d,\"ssid\":\"%s\",\"rssi\":%d}", bss_link->authmode, ssid, bss_link->rssi);
			MQTT_Publish(&mqttClient, tBuf, dBuf, os_strlen(dBuf), 0, 0);
			bss_link = bss_link->next.stqe_next;
		}
	} else {
		iprintf(RESPONSE, "WIFI scan failed %d\n", status);
	}
}

void ICACHE_FLASH_ATTR user_init(void)
{
	system_set_os_print(0);
	petes_initialisation(); // Sets up the ports and initialises various values needed by the MQTT call backs, time etc
	setupwebpage_init(); // Set up the configuration/control web pages - or not

}
