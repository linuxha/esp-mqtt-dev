/*
/* config.c
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
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
#include "espmissingincludes.h"
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "mqtt.h"
#include "config.h"
#include "user_config.h"

extern int enable_debug_messages; // Needed by debug.h
#include "debug.h"

//#include <rboot-api.h>

SYSCFG sysCfg;
SAVE_FLAG saveFlag;


/*uint32 ICACHE_FLASH_ATTR cfgGetConfigSector() {
	uint32 x;
	rboot_config bootconf = rboot_get_config();
	x = bootconf.roms[bootconf.current_rom]; // flash address of rom
	x /= 0x100000;     // flash mb containing rom
	x *= 0x100;        // first sector of flash mb containing rom
	x += CFG_LOCATION; // sector of config
	return x;
}*/

void ICACHE_FLASH_ATTR
cfgSave()
{
		//system_param_save_with_protect(cfgGetConfigSector(),(uint32 *)&sysCfg, sizeof(SYSCFG));
		system_param_save_with_protect(CFG_LOCATION,(uint32 *)&sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 0;

}

void ICACHE_FLASH_ATTR
cfgLoad(int force)
{

	iprintf(DEBUG,"\r\nConfig load ...\r\n");
	//system_param_load(cfgGetConfigSector(),0,(uint32 *)&sysCfg, sizeof(SYSCFG));
	system_param_load(CFG_LOCATION,0,(uint32 *)&sysCfg, sizeof(SYSCFG));
	if((sysCfg.cfg_holder != CFG_HOLDER)||(force==1)){
		os_memset(&sysCfg, 0x00, sizeof sysCfg);
		sysCfg.cfg_holder = CFG_HOLDER;
		os_sprintf(sysCfg.stationSsid, "%s", WIFI_CLIENTSSID);
		os_sprintf(sysCfg.stationPwd, "%s", WIFI_CLIENTPASSWORD);

		os_sprintf(sysCfg.stationSsid2, "%s", WIFI_CLIENTSSID);
		os_sprintf(sysCfg.stationPwd2, "%s", WIFI_CLIENTPASSWORD);
		sysCfg.stationSwap=0;
		sysCfg.stationType = STA_TYPE;

		os_sprintf(sysCfg.deviceId, MQTT_CLIENT_ID, system_get_chip_id());
		os_sprintf(sysCfg.mqttHost, "%s", MQTT_HOST);
		sysCfg.mqttPort = MQTT_PORT;
		os_sprintf(sysCfg.mqttUser, "%s", MQTT_USER);
		os_sprintf(sysCfg.mqttPass, "%s", MQTT_PASS);
		os_strcpy(sysCfg.base,sysCfg.deviceId); //use device unique ID
	    //os_sprintf(sysCfg.base, "%s", MQTT_BASE);
	    os_sprintf(sysCfg.description,"%s","empty");

	    // Aidans
		os_sprintf(sysCfg.webUser, "admin"); // Default web password
		os_sprintf(sysCfg.webPassword, "password"); // Default web password
		sysCfg.enableWebPageControl = 1; // Enable webpage control
		// END Aidans

		sysCfg.security = DEFAULT_SECURITY;	/* default non ssl */

		sysCfg.mqttKeepalive = MQTT_KEEPALIVE;

		sysCfg.dawn=DEFAULT_DAWN;
		sysCfg.dusk=DEFAULT_DUSK;
		sysCfg.peak=DEFAULT_PEAK;
		sysCfg.offPeak=DEFAULT_OFF_PEAK;
		sysCfg.frost=DEFAULT_FROST;
		sysCfg.onOne=DEFAULT_ON_1;
		sysCfg.offOne=DEFAULT_OFF_1;
		sysCfg.onTwo=DEFAULT_ON_2;
		sysCfg.offTwo=DEFAULT_OFF_2;
		sysCfg.out0Status=DEFAULT_0_STATUS;
		sysCfg.out4Status=DEFAULT_4_STATUS;
		sysCfg.out5Status=DEFAULT_5_STATUS;
		sysCfg.out15Status=DEFAULT_15_STATUS;
		sysCfg.out12Status=DEFAULT_12_STATUS;
		sysCfg.out2Status=DEFAULT_2_STATUS;
		sysCfg.out13Status=DEFAULT_13_STATUS;
		sysCfg.in14Bounce=DEFAULT_14_IN;
		sysCfg.in2Bounce=DEFAULT_2_IN;
		sysCfg.sensor_type=DEFAULT_TEMP_TYPE;
		sysCfg.enable13=THIRTEEN_ENABLE;
        sysCfg.override0=DEFAULT_MANUAL;
        sysCfg.override0Time=DEFAULT_MANUAL_TIME;
        sysCfg.invert=DEFAULT_INVERT;
        sysCfg.newInvert=DEFAULT_INVERT;
        sysCfg.calibrate = CALIBRATE;
        sysCfg.wifiSetup=0;
        sysCfg.clock=255;
        sysCfg.serial2=0;
        sysCfg.ssid_retries=DEFAULT_RETRIES;
		// AIDANS
		os_sprintf(sysCfg.webUser, "admin"); // Default web password
		os_sprintf(sysCfg.webPassword, "password"); // Default web password
		sysCfg.enableWebPageControl = 1; // Enable webpage control
		// aidan end
		
		os_sprintf(sysCfg.otaHost, "%s", OTA_HOST);
		sysCfg.otaPort = OTA_PORT;
		sysCfg.out16Status=DEFAULT_16_STATUS;
		sysCfg.relayoutput_override=0;
		sysCfg.system_clock=DEFAULT_CLOCK;
		sysCfg.wifi_button=2;
		sysCfg.sonoff=0;

		sysCfg.timeoutPeriod=0;
		sysCfg.timeoutSecs=0;
		sysCfg.timeoutRepeat=0;
		sysCfg.timeoutPin=0;
		sysCfg.timeoutInvert=0;
		sysCfg.electrodragon=0;
        sysCfg.temperaturePort=TEMPERATURE_PORT;
        sysCfg.set2Out=0;
        sysCfg.rgbPermPort=-1;
        sysCfg.rgbPermRed=0;
        sysCfg.rgbPermGreen=0;
        sysCfg.rgbPermBlue=0;
        sysCfg.rgbPermNum=0;
        sysCfg.OTAdone=0;
        sysCfg.enable16=0;
        sysCfg.iliGo=0;
        sysCfg.out14Enable=0;
        sysCfg.out14Status=0;

		iprintf(DEBUG,"Default configuration stored\r\n");

		cfgSave();
	}

}
