#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#define CFG_HOLDER	0x00FF55A2	// Change this value to load default configurations
#define CFG_LOCATION	0x79	// Please don't change unless you know what you doing

// Aidans webpage setup
#define USE_WIFI_MODE		STATIONAP_MODE
#ifndef WIFI_CLIENTSSID
	#define WIFI_CLIENTSSID		"beyond"
#endif
#ifndef WIFI_CLIENTPASSWORD
	#define WIFI_CLIENTPASSWORD	"1921682012"
#endif
#ifndef WIFI_AP_NAME
	#define WIFI_AP_NAME		"hack-Setup"
#endif
#ifndef WIFI_AP_PASSWORD
	#define WIFI_AP_PASSWORD	""
#endif
#define STA_TYPE AUTH_WPA2_PSK
// Aidans webpage setup - END

#define SYSTEM_VER "1.7.2"  // important as this shows up in {ver?} command

#define RGB_BUFFSIZE 900   // all down to RAM really - 300 LEDS max
#define SERIAL_BUFFERS 128 // there are two of them - one temporary for building up strings
#define TEMPERATURE_READING_DELAY 30000 /* milliseconds for temperature 30 second intervals */
#define TBUFSIZE 84

#define MQTT_HOST			"192.168.0.20"
#define MQTT_PORT			1883
#define MQTT_BUF_SIZE		1024
#define MQTT_KEEPALIVE		120	 // seconds

#define MQTT_CLIENT_ID		"ARPS_%08X"
#define MQTT_USER			"admin"
#define MQTT_PASS			"pass"
#define MQTT_BASE  "999"

#define DEFAULT_DAWN  480 // 8am - minutes since midnight
#define DEFAULT_DUSK  1080 // 6pm - minutes since midnight
#define DEFAULT_PEAK   23  // temperature
#define DEFAULT_OFF_PEAK 19
#define DEFAULT_FROST  14
#define DEFAULT_ON_1  480  // 8am morning
#define DEFAULT_OFF_1  720 // 12 mid-day
#define DEFAULT_ON_2  900  // 3pm
#define DEFAULT_OFF_2 1380 // 11pm
#define DEFAULT_0_STATUS 0 // output 0=OFF, 1=ON, 2=ON TILL MIDNIGHT 3= ON TILL DAWN 4=HEAT
#define DEFAULT_4_STATUS 0
#define DEFAULT_5_STATUS 0
#define DEFAULT_15_STATUS 0
#define DEFAULT_16_STATUS 0
#define DEFAULT_12_STATUS 0
#define DEFAULT_2_STATUS 0
#define DEFAULT_13_STATUS 0
#define DEFAULT_14_IN 5
#define DEFAULT_2_IN 5
#define DEFAULT_TEMP_TYPE 0 // 0 for Dallas, 1 for DHT22
#define TEMPERATURE_PORT 2      // default port bit for temperature chips
#define THIRTEEN_ENABLE 0  // not an output - used to indicate clock working
#define MQTT_RECONNECT_TIMEOUT 	5	/*second*/
#define DEFAULT_MANUAL 0   // manual override option off
#define DEFAULT_MANUAL_TIME 120 * 60 * 20   // 120 minutes before reverting to normal
#define DEFAULT_INVERT 1 // invert output for GPIO0  if 1 is inverted ie HIGH=OFF
#define DEFAULT_RETRIES 30 // number of SSID retries before trying the alternative SSID
// Default ADC calibration value
#define CALIBRATE 478
#define DEFAULT_CLOCK 80 // clock speed

// #define CLIENT_SSL_ENABLE
#define DEFAULT_SECURITY 0
#define QUEUE_BUFFER_SIZE 2048

#define PROTOCOL_NAMEv31	/*MQTT version 3.1 compatible with Mosquitto v0.15*/
//PROTOCOL_NAMEv311			/*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

#define RGBMAX 600
#define TICKS 6000 // REBOOTING NOT GOOD - 4 secs a count - messes up IO so only after a LONG time of no WIFI do we reboot

#define OUT_ON 1
#define OUT_OFF 0

#ifndef OTA_HOST
 #define OTA_HOST "www.scargill.net"
#endif
#ifndef OTA_PORT
 #define OTA_PORT 80
#endif

#endif
