/*
 * aidans_code.c
 *
 *  Created on: 16 Mar 2015
 *      Author: Aidan
 */

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
#include "aidans.h"
#include "cgiwifi.h"


/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/

void  IFA setupwebpage_init(void)
{
int reset_count = 0;
if (sysCfg.wifiSetup == 0) // OK button not held down at power up, setup MQTT etc
	{
    wifi_station_set_hostname("THEmagic");
	wifiInit(STATION_MODE); // Only connect to the local network
	iprintf(INFO,"Now in STATION mode\r\n");
	if (sysCfg.enableWebPageControl)
		{
	    iprintf(INFO,"Web page control is enabled\r\n");
		builtInUrls[0].cgiArg = "/";
		httpdInit(builtInUrls, 80);
		}
	else iprintf(INFO,"Web page control disabled\r\n");
	connectStatus=16; // Allow Pete's code to check


	}
else
	{

	sysCfg.wifiSetup=0; cfgSave();
	//struct softap_config apConfig;
	iprintf(INFO,"Web Page Configuration mode\r\n");
	httpdInit(builtInUrls, 80);
	wifiInit(STATIONAP_MODE); // Connect to the local network
	// Scan for local access points - added to remove the need for a separate webpage setup file
	iprintf(INFO, "\n\nScanning for wifi\n\n");
//	while (lostThePlotTimer > 0); // wait approx' 15 seconds and then scan for wifi networks

//	while (cgiWifiAps.scanInProgress); // Make sure that there isn;t any other scan going on
//	cgiWifiAps.scanInProgress = 1;
//	wifi_station_scan(NULL, wifiScanDoneCb);
	}
}


// Aidans END


