/*
Cgi/template routines for the /wifi url.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "espmissingincludes.h" // ncherry@linuxha.com

#include "stdint.h"

extern int enable_debug_messages; // Needed by debug.h
#include "debug.h"

#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "cgi.h"
//#include "espmissingincludes.h"

#include "config.h"

#include "cgiwifi.h"
extern SYSCFG sysCfg;

// ncherry@linuxha.com
// more of this silliness
extern void ets_delay_us(int);
extern void iprintf(uint16_t debug_type, char *fmt, ... );
//#define os_strcpy strcpy

//Enable this to disallow any changes in AP settings
//#define DEMO_MODE


//Static scan status storage.
ScanResultData cgiWifiAps;

//Callback the code calls when a wlan ap scan is done. Basically stores the result in
//the cgiWifiAps struct.
void ICACHE_FLASH_ATTR wifiScanDoneCb(void *arg, STATUS status) {
	int n;
	struct bss_info *bss_link = (struct bss_info *)arg;

	if (status!=OK) {
		cgiWifiAps.scanInProgress=0;
		return;
	}

	//Clear prev ap data if needed.
	if (cgiWifiAps.apData!=NULL) {
		for (n=0; n<cgiWifiAps.noAps; n++) os_free(cgiWifiAps.apData[n]);
		os_free(cgiWifiAps.apData);
	}

	//Count amount of access points found.
	n=0;
	while (bss_link != NULL) {
		bss_link = bss_link->next.stqe_next;
		n++;
	}
	//Allocate memory for access point data
	cgiWifiAps.apData=(ApData **)os_malloc(sizeof(ApData *)*n);
	cgiWifiAps.noAps=n;
//	INFO("Scan done: found %d APs\n", n);

	//Copy access point data to the static struct
	n=0;
	bss_link = (struct bss_info *)arg;
	while (bss_link != NULL)
		{
		if (n>=cgiWifiAps.noAps)
			{
			//This means the bss_link changed under our nose. Shouldn't happen!
			//Break because otherwise we will write in unallocated memory.
//			INFO("Huh? I have more than the allocated %d aps!\n", cgiWifiAps.noAps);
			break;
			}
		//Save the ap data.
		cgiWifiAps.apData[n]=(ApData *)os_malloc(sizeof(ApData));
		cgiWifiAps.apData[n]->rssi=bss_link->rssi;
		cgiWifiAps.apData[n]->enc=bss_link->authmode;
		strncpy(cgiWifiAps.apData[n]->ssid, (char*)bss_link->ssid, 32);

			{
			char s[120];
			os_sprintf(s, "%s\r\n", cgiWifiAps.apData[n]->ssid);
			iprintf(INFO, s);
			}

		bss_link = bss_link->next.stqe_next;
		n++;
		}
	//We're done.
	cgiWifiAps.scanInProgress=0;
}


//Routine to start a WiFi access point scan.
static void ICACHE_FLASH_ATTR wifiStartScan() {
//	int x;
	if (cgiWifiAps.scanInProgress) return;
	cgiWifiAps.scanInProgress=1;
	wifi_station_scan(NULL, wifiScanDoneCb);
}

//This CGI is called from the bit of AJAX-code in wifi.tpl. It will initiate a
//scan for access points and if available will return the result of an earlier scan.
//The result is embedded in a bit of JSON parsed by the javascript in wifi.tpl.
int ICACHE_FLASH_ATTR cgiWiFiScan(HttpdConnData *connData) {
	int len;
	int i;
	char buff[1024];
	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);

	if (cgiWifiAps.scanInProgress==1) {
		//We're still scanning. Tell Javascript code that.
		len=os_sprintf(buff, "{\n \"result\": { \n\"inProgress\": \"1\"\n }\n}\n");
		httpdSend(connData, buff, len);
	} else {
		//We have a scan result. Pass it on.
		len=os_sprintf(buff, "{\n \"result\": { \n\"inProgress\": \"0\", \n\"APs\": [\n");
		httpdSend(connData, buff, len);
		if (cgiWifiAps.apData==NULL) cgiWifiAps.noAps=0;
		for (i=0; i<cgiWifiAps.noAps; i++) {
			//Fill in json code for an access point
			len=os_sprintf(buff, "{\"essid\": \"%s\", \"rssi\": \"%d\", \"enc\": \"%d\"}%s\n",
					cgiWifiAps.apData[i]->ssid, cgiWifiAps.apData[i]->rssi,
					cgiWifiAps.apData[i]->enc, (i==cgiWifiAps.noAps-1)?"":",");
			httpdSend(connData, buff, len);
		}
		len=os_sprintf(buff, "]\n}\n}\n");
		httpdSend(connData, buff, len);
		//Also start a new scan.
		wifiStartScan();
	}
	return HTTPD_CGI_DONE;
}

//Temp store for new ap info.
static struct station_config stconf;


//This routine is ran some time after a connection attempt to an access point. If
//the connect succeeds, this gets the module in STA-only mode.
static void ICACHE_FLASH_ATTR resetTimerCb(void *arg) {
	int x=wifi_station_get_connect_status();
	if (x==STATION_GOT_IP) {
		//Go to STA mode. This needs a reset, so do that.
//		INFO("Got IP. Going into STA mode..\n");
		wifi_set_opmode(1);
		system_restart();
	} else {
//		INFO("Connect fail. Not going into STA-only mode.\n");
		//Maybe also pass this through on the webpage?
	}
}

//Actually connect to a station. This routine is timed because I had problems
//with immediate connections earlier. It probably was something else that caused it,
//but I can't be arsed to put the code back :P
static void ICACHE_FLASH_ATTR reassTimerCb(void *arg) {
	int x;
	static ETSTimer resetTimer;
//	INFO("Try to connect to AP....\n");
	wifi_station_disconnect();
	wifi_station_set_config(&stconf);
	wifi_station_connect();
	x=wifi_get_opmode();
	if (x!=1) {
		//Schedule disconnect/connect
		os_timer_disarm(&resetTimer);
		os_timer_setfn(&resetTimer, resetTimerCb, NULL);
		os_timer_arm(&resetTimer, 4000, 0);
	}
}



static ICACHE_FLASH_ATTR void removeNonalpha(uint8_t *s)
{
while (*s)
	if (((*s >= 'a') && (*s <= 'z' )) || ((*s >= 'A') && (*s <= 'Z' )) || ((*s >= '0') && (*s <= '9')) || (*s == ' ') || (*s == '{') || (*s == '}') || (*s == ',') || (*s == ':') || (*s == '=') || (*s == '_') || (*s == '-') || (*s == ','))
		++s;
	else
		*s++ = '_';
}

//This cgi uses the routines above to connect to a specific access point with the
//given ESSID using the given password.
int ICACHE_FLASH_ATTR cgiWiFiConnect(HttpdConnData *connData) {
	//uint8_t essid[32];
	char essid[32];
	//char passwd[32];
	char mqtt_port[7], ota_port[7];
	char wifi_button[3], sonoff[3];
	//char ota_host[64];
	char s[16];
	//int i;

	static ETSTimer reassTimer;
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}
	
	// Get the arguments from the response

	if (httpdFindArg(connData->postBuff, "essid", essid, sizeof(essid)) != -1) { // If no SSID set, use the last one
	    if (os_strlen(essid) > 0) { // None null SSID?
		os_strcpy((char *) sysCfg.stationSsid, essid); // Save the SSID into the main variables structure
	    }
	    os_strcpy((char *) sysCfg.stationSsid2, essid); // Save the SSID into the main variables structure - temporary
	}


	httpdFindArg(connData->postBuff, "sta_pwd", (char *) sysCfg.stationPwd, sizeof(sysCfg.stationPwd));

	if (sysCfg.stationSwap==1) {
	    os_strncpy((char *) stconf.ssid, (char *) sysCfg.stationSsid2, os_strlen((char *) sysCfg.stationSsid2));
	    os_strncpy((char*)stconf.password, (char *) sysCfg.stationPwd2, os_strlen((char *) sysCfg.stationPwd2));
	    iprintf(INFO,"\r\nCONNECT: SSID=%s, PWD=%s\r\n",sysCfg.stationSsid2, sysCfg.stationPwd2);
	} else {
	    os_strncpy((char*)stconf.ssid, (char *) sysCfg.stationSsid, os_strlen((char *) sysCfg.stationSsid));
	    os_strncpy((char*)stconf.password, (char *) sysCfg.stationPwd, os_strlen((char *) sysCfg.stationPwd));
	    iprintf(INFO,"\r\nCONNECT: SSID=%s, PWD=%s\r\n",sysCfg.stationSsid, sysCfg.stationPwd);
	}

	// Save the ssid and password
	//os_sprintf(sysCfg.sta_ssid, "%s", stconf.ssid);
	//os_sprintf(sysCfg.sta_pwd, "%s", stconf.password);

	httpdFindArg(connData->postBuff, "ssid2", (char *) sysCfg.stationSsid2, sizeof(sysCfg.stationSsid2));
	httpdFindArg(connData->postBuff, "pass2", (char *) sysCfg.stationPwd2, sizeof(sysCfg.stationPwd2));
	httpdFindArg(connData->postBuff, "mqtt_pass", (char *) sysCfg.mqttPass, sizeof(sysCfg.mqttPass));
	httpdFindArg(connData->postBuff, "mqtt_user", (char *) sysCfg.mqttUser, sizeof(sysCfg.mqttUser));
	httpdFindArg(connData->postBuff, "mqtt_host", (char *) sysCfg.mqttHost, sizeof(sysCfg.mqttHost));

	httpdFindArg(connData->postBuff, "mqtt_device_name", (char *) sysCfg.base, sizeof(sysCfg.base));
	// Remove any spaces in the device name and replace with underscores
	uint8_t *st;
	st = sysCfg.base;
	while (*st) {
	    if (*st == ' ') {
		*st = '_';
	    }
	    ++st;
	}

	httpdFindArg(connData->postBuff, "mqtt_device_description", (char *) sysCfg.description, sizeof(sysCfg.description));
	httpdFindArg(connData->postBuff, "mqtt_device_attribute", (char *) sysCfg.attribute, sizeof(sysCfg.attribute));

	httpdFindArg(connData->postBuff, "mqtt_port", mqtt_port, sizeof(mqtt_port));
	sysCfg.mqttPort = atoi(mqtt_port); // Convert mqtt_port string to integer


	httpdFindArg(connData->postBuff, "wifi_button", wifi_button, sizeof(wifi_button));
	sysCfg.wifi_button = atoi(wifi_button); // Convert wifi_button string to integer


	httpdFindArg(connData->postBuff, "sonoff", sonoff, sizeof(sonoff));
	sysCfg.sonoff = atoi(sonoff); // Convert mqtt_port string to integer


	httpdFindArg(connData->postBuff, "ota_port", ota_port, sizeof(ota_port));
	sysCfg.otaPort = atoi(ota_port); // Convert mqtt_port string to integer

	httpdFindArg(connData->postBuff, "ota_host", (char *) sysCfg.otaHost, sizeof(sysCfg.otaHost));

	// Web control stuff
	httpdFindArg(connData->postBuff, "web_user", (char *) sysCfg.webUser, sizeof(sysCfg.webUser));
	httpdFindArg(connData->postBuff, "web_pass", (char *) sysCfg.webPassword, sizeof(sysCfg.webPassword));

	// Remove any dodgy characters from the various parameters
	removeNonalpha(sysCfg.base);
	removeNonalpha(sysCfg.description);
//	removeNonalpha(sysCfg.attribute);


	// Enable/disable webpage control
	httpdFindArg(connData->postBuff, "enable_webpage_control", s, sizeof(s));
	if (!os_strcmp(s, "yes"))
		sysCfg.enableWebPageControl = 1;
	else
		sysCfg.enableWebPageControl = 0;


	cfgSave();
	iprintf(INFO,"\r\nSSID=%s, WiFi Pwd=%s,Host=%s,Port= %d,User=%s,PW=%s, Device=%s, Description=%s",  \
			(sysCfg.stationSwap)? sysCfg.stationSsid2 : sysCfg.stationSsid , (sysCfg.stationSwap)? sysCfg.stationPwd2:sysCfg.stationPwd, sysCfg.mqttHost, sysCfg.mqttPort, sysCfg.mqttUser, sysCfg.mqttPass, sysCfg.base, sysCfg.description, sysCfg.attribute);


//	INFO("Trying to connect to AP %s pw %s\n", essid, passwd);


	//Schedule disconnect/connect
	os_timer_disarm(&reassTimer);
	os_timer_setfn(&reassTimer, reassTimerCb, NULL);
//Set to 0 if you want to disable the actual reconnecting bit
#ifdef DEMO_MODE
	httpdRedirect(connData, "/wifi");
#else
	os_timer_arm(&reassTimer, 1000, 0);
	httpdRedirect(connData, "connecting.html");
#endif
	return HTTPD_CGI_DONE;
}

//This cgi uses the routines above to connect to a specific access point with the
//given ESSID using the given password.
int ICACHE_FLASH_ATTR cgiWifiSetMode(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->getArgs, "mode", buff, sizeof(buff));
	if (len!=0) {
//		INFO("cgiWifiSetMode: %s\n", buff);
#ifndef DEMO_MODE
		wifi_set_opmode(atoi(buff));
		system_restart();
#endif
	}
	httpdRedirect(connData, "/wifi");
	return HTTPD_CGI_DONE;
}

//Template code for the WLAN page.
void ICACHE_FLASH_ATTR tplWlan(HttpdConnData *connData, char *token, void **arg)
{
char buff[1024];
int x;
char s[32];
//static struct station_config stconf;
if (token==NULL) return;
//wifi_station_get_config(&stconf);

os_strcpy(buff, "Unknown");
if (os_strcmp(token, "WiFiMode")==0)
	{
	x=wifi_get_opmode();
	if (x==1) os_strcpy(buff, "Client");
	if (x==2) os_strcpy(buff, "SoftAP");
	if (x==3) os_strcpy(buff, "STA+AP");
	}
else
	if (os_strcmp(token, "mqtt_host")==0)
		{
		os_strcpy(buff, (char*) sysCfg.mqttHost);
		}
else
	if (os_strcmp(token, "ota_host")==0)
			{
			os_strcpy(buff, (char*) sysCfg.otaHost);
			}
		else
	if (os_strcmp(token, "ota_port")==0)
			{
			os_sprintf(s,"%d", sysCfg.otaPort);
			os_strcpy(buff, (char*) s);
			}

	else
	if (os_strcmp(token, "wifi_button")==0)
			{
			os_sprintf(s,"%d", sysCfg.wifi_button);
			os_strcpy(buff, (char*) s);
			}

	else
	if (os_strcmp(token, "sonoff")==0)
		{
		os_sprintf(s,"%d", sysCfg.sonoff);
		os_strcpy(buff, (char*) s);
		}

	else
		if (os_strcmp(token, "ssid2")==0)
			{
		   os_strcpy(buff, (char*) sysCfg.stationSsid2);
			}
	else
		if (os_strcmp(token, "pass2")==0)
			{
			os_strcpy(buff, (char*) sysCfg.stationPwd2);
			}
else
	if (os_strcmp(token, "mqtt_port")==0)
		{
		os_sprintf(s,"%d", sysCfg.mqttPort);
		os_strcpy(buff, (char*) s);
		}
else
	if (os_strcmp(token, "mqtt_pass")==0)
		{
		os_strcpy(buff, (char*) sysCfg.mqttPass);
		}
else
	if (os_strcmp(token, "mqtt_user")==0)
		{
		os_strcpy(buff, (char*) sysCfg.mqttUser);
		}

else
	if (os_strcmp(token, "mqtt_device_name")==0)
		{
		os_strcpy(buff, (char*) sysCfg.base);
		}
else
	if (os_strcmp(token, "mqtt_device_description")==0)
		{
		os_strcpy(buff, (char*) sysCfg.description);
		}
else
	if (os_strcmp(token, "mqtt_device_attribute")==0)
		{
		os_strcpy(buff, (char*) sysCfg.attribute);
		}
else
	if (os_strcmp(token, "web_pass")==0)
		{
		os_strcpy(buff, (char*) sysCfg.webPassword);
		}
else
	if (os_strcmp(token, "sta_ssid") == 0)
		{
		os_strcpy(buff, (sysCfg.stationSwap)? (char*) sysCfg.stationSsid2:(char*) sysCfg.stationSsid);
		}
else
	if (os_strcmp(token, "web_enabled_yes") == 0)
		{
		if  (sysCfg.enableWebPageControl == 1)
			os_strcpy(buff, " checked");
		}
if (os_strcmp(token, "web_enabled_no") == 0)
	{
	if  (sysCfg.enableWebPageControl == 0)
		os_strcpy(buff, " checked");
	}
else
	if (os_strcmp(token, "sta_pwd")==0)
		{
		os_strcpy(buff, (sysCfg.stationSwap)? (char*)sysCfg.stationPwd2:(char*)sysCfg.stationPwd);
		}
else
	if (os_strcmp(token, "WiFiapwarn")==0)
		{
		x=wifi_get_opmode();
		if (x==2)
			{
			os_strcpy(buff, "<b>Can't scan in this mode.</b> Click <a href=\"setmode.cgi?mode=3\">here</a> to go to STA+AP mode.");
			}
		else
			{
			os_strcpy(buff, "Click <a href=\"setmode.cgi?mode=2\">here</a> to go to standalone AP mode.");
			}
		}

httpdSend(connData, buff, -1);
}


