#include "espmissingincludes.h" // ncherry@linuxha.com

#include "osapi.h"
#include "user_interface.h"
#include "user_config.h"
#include "wifi.h"
//#include "espmissingincludes.h"

extern int enable_debug_messages; // Needed by debug.h
#include "debug.h"

#include "config.h"

const char *WiFiMode[] =
{
		"NULL",		// 0x00
		"STATION",	// 0x01
		"SOFTAP", 	// 0x02
		"STATIONAP"	// 0x03
};

void IFA wifiInit(int wifiMode)
{
	iprintf(DEBUG,"\r===== WiFi Init =====\r");

	wifi_set_opmode(0); // Clear all modes
	iprintf(DEBUG,"\r\nSetting WiFI\r\n");

	if(wifiMode & SOFTAP_MODE)
		{
	iprintf(DEBUG,"\rSetting SOFTAP Mode\r\n");
		setup_wifi_ap_mode();
		iprintf(DEBUG,"Done\r\n");
		}

	if(wifiMode & STATION_MODE)
		{
	iprintf(DEBUG,"\rSetting Station Mode \r\n");
		setup_wifi_st_mode();
		 wifi_station_set_reconnect_policy(1);
		iprintf(DEBUG,"Done\r\n");
		}

	if(wifi_get_phy_mode() != PHY_MODE_11N)
		wifi_set_phy_mode(PHY_MODE_11N);
	if(wifi_station_get_auto_connect() == 0)
		wifi_station_set_auto_connect(1);

	iprintf(DEBUG,"Wi-Fi mode: %s\r\n", WiFiMode[wifi_get_opmode()]);
	if(wifiMode & SOFTAP_MODE)
		{
		struct softap_config apConfig;
		if(wifi_softap_get_config(&apConfig))
			{
		iprintf(DEBUG,"AP config: SSID: %s, PASSWORD: %s\r\n",
				apConfig.ssid,
				apConfig.password);
			}
		}
	if(wifiMode & STATION_MODE)
		{
		struct station_config stationConfig;
		if(wifi_station_get_config(&stationConfig))
			{
		iprintf(DEBUG,"STA config: SSID: %s, PASSWORD: %s\r\n",
				stationConfig.ssid,
				stationConfig.password);
			}
		}
	}



void IFA setup_wifi_ap_mode(void)
{
  wifi_set_opmode((wifi_get_opmode() | SOFTAP_MODE));
  //wifi_set_opmode(STATIONAP_MODE);

  struct softap_config apconfig;
  if(wifi_softap_get_config(&apconfig))
	{
    iprintf(DEBUG,"In softw AP Setup\r\n");
	wifi_softap_dhcps_stop();
	os_memset(apconfig.ssid, 0, sizeof(apconfig.ssid));
	os_memset(apconfig.password, 0, sizeof(apconfig.password));

	apconfig.ssid_len = os_sprintf(apconfig.ssid, WIFI_AP_NAME);
	os_sprintf(apconfig.password, "%s", WIFI_AP_PASSWORD);
	apconfig.authmode = AUTH_OPEN; // AUTH_WPA_WPA2_PSK - for protected AP
	apconfig.ssid_hidden = 0;
	apconfig.channel = 7;
	apconfig.max_connection = 4;
	if(!wifi_softap_set_config(&apconfig))
		{
	iprintf(DEBUG,"\r\n === ESP8266 not set AP config!\r\n");
		}
	else
	iprintf(DEBUG,"\r == ESP Set to AP Mode for Setup - 192.168.4.1\r");

	struct ip_info ipinfo;
	wifi_get_ip_info(SOFTAP_IF, &ipinfo);
	IP4_ADDR(&ipinfo.ip, 192, 168, 4, 1);
	IP4_ADDR(&ipinfo.gw, 192, 168, 4, 1);
	IP4_ADDR(&ipinfo.netmask, 255, 255, 255, 0);
	wifi_set_ip_info(SOFTAP_IF, &ipinfo);
	wifi_softap_dhcps_start();
	}
  else
	{
  iprintf(DEBUG,"\r\n === Problems setting AP Mode\r\n");
	}

  iprintf(DEBUG,"\r\n === ESP8266 in AP mode configured.\r\n");
}

void IFA setup_wifi_st_mode(void)
{
   // wifi_set_opmode((wifi_get_opmode()|STATION_MODE)&STATIONAP_MODE);
	wifi_set_opmode(wifi_get_opmode() | STATION_MODE);

	struct station_config stconfig;
	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
	if(wifi_station_get_config(&stconfig))
		{
		os_memset(stconfig.ssid, 0, sizeof(stconfig.ssid));
		os_memset(stconfig.password, 0, sizeof(stconfig.password));
		if (strlen(sysCfg.stationSsid2)==0) sysCfg.stationSwap=0;
		if (sysCfg.stationSwap==1)
			{
					os_sprintf(stconfig.ssid, "%s", sysCfg.stationSsid2);
					os_sprintf(stconfig.password, "%s", sysCfg.stationPwd2);
			}
		else
			{
					os_sprintf(stconfig.ssid, "%s", sysCfg.stationSsid);
					os_sprintf(stconfig.password, "%s", sysCfg.stationPwd);
			}
		if(!wifi_station_set_config(&stconfig))
			{
			#ifdef PLATFORM_DEBUG
			INFO("Station config not correctly configured!\r\n");
			#endif
			}
		}

	wifi_station_connect();
	wifi_station_dhcpc_start();
	wifi_station_set_auto_connect(1);
	#ifdef PLATFORM_DEBUG
	INFO("ESP8266 in STATION mode.\r\n");
	#endif
}

