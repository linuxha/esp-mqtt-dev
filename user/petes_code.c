/*
   petes_code.c - this is the main control page
   Created: July 2015 - November 2016
   Authors: Aidan and Peter (this page Peter Scargill - http://tech.scargill.net)
*/

#include "user_config.h"
#include "aidan_and_petes.h"	// wsBitMask defined here but gives error on previous compiles ncherry@linuxha.com
#include "petes.h"
#include "easygpio/easygpio.h"

#include <math.h>

#include "../driver/bme280.h"
#include "../driver/si1132.h"
#include "../include/driver/rgb_lcd.h"

#include "rboot-ota.h"
#include "softuart.h"
#include "include/driver/spi.h"

static uint8_t wsBitMask;	// needed to be added ncherry@linuxha.com

void IFA setFlashBack(uint8_t r, uint8_t g, uint8_t b, uint8_t duration);
void IFA mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len);
void IFA mqtt_setup();
void IFA mqtt_init();

int16_t heartBeat = TICKS - 1; 	// ticks before forcing a reset
uint8_t connectStatus = 0; 			// i.e. what status are we in - do we need to connect MQTT etc?

uint32_t quick_time=0;

Softuart softuart2;

// more on pin definitions here https://esp8266.ru/esp8266-pin-register-strapping/

#define PWM_12_OUT_IO_MUX PERIPHS_IO_MUX_MTDI_U
#define PWM_12_OUT_IO_NUM 12
#define PWM_12_OUT_IO_FUNC FUNC_GPIO12

#define PWM_14_OUT_IO_MUX PERIPHS_IO_MUX_MTMS_U
#define PWM_14_OUT_IO_NUM 14
#define PWM_14_OUT_IO_FUNC FUNC_GPIO14

#define PWM_15_OUT_IO_MUX PERIPHS_IO_MUX_MTDO_U
#define PWM_15_OUT_IO_NUM 15
#define PWM_15_OUT_IO_FUNC FUNC_GPIO15

#define PWM_13_OUT_IO_MUX PERIPHS_IO_MUX_MTCK_U
#define PWM_13_OUT_IO_NUM 13
#define PWM_13_OUT_IO_FUNC FUNC_GPIO13


#define PWM_4_OUT_IO_MUX PERIPHS_IO_MUX_GPIO4_U
#define PWM_4_OUT_IO_NUM 4
#define PWM_4_OUT_IO_FUNC FUNC_GPIO4

#define PWM_5_OUT_IO_MUX PERIPHS_IO_MUX_GPIO5_U
#define PWM_5_OUT_IO_NUM 5
#define PWM_5_OUT_IO_FUNC FUNC_GPIO5

#define PWM_CHANNEL 5

#define INDIC_START 60,40,0
#define INDIC_B_GOT_IP 50,100,0,10
#define INDIC_B_LOST_IP 50,0,50,10
#define INDIC_B_GOTMQTT 50,50,0,40
#define INDIC_B_LOSTMQTT 100,50,0,40

#define INDIC_TIME 0,40,0
#define INDIC_NO_TIME 0,0,20

#define INDIC_B_WEB_BUTTON_PRESSED 200,0,200,40
#define INDIC_WEB 0,0,40
#define INDIC_POWER 20,20,20

#define INDIC_B_ATTEMPTING_OTA 0,0,200,40
#define INDIC_B_FAILED_OTA 200,0,0,100
#define INDIC_B_SUCCEEDED_OTA 0,200,0,40

#define NORMAL_LED_OFF_TIME 40

#define LCD_BL_PIN 0b1000

uint8_t flash_buffer[4096]; 	// flash testing buffer - that's a lot of room to use up

uint32 duty[] = {0, 0, 0};
uint32 freq = 1000;
static uint8 donepwm = 0;

uint8_t flashQue[16][4];
uint8_t flashIn, flashOut;
uint8_t flashMainR, flashMainG, flashMainB;

uint32 io_info[][3] = {
	{PWM_15_OUT_IO_MUX, PWM_15_OUT_IO_FUNC, PWM_15_OUT_IO_NUM},
	{PWM_4_OUT_IO_MUX, PWM_4_OUT_IO_FUNC, PWM_4_OUT_IO_NUM},
	{PWM_5_OUT_IO_MUX, PWM_5_OUT_IO_FUNC, PWM_5_OUT_IO_NUM}
};

uint16_t timeoutCount;

uint8_t IFA hexdec(uint8_t hex) {
	if ((hex >= 'a') && (hex <= 'f')) return hex - 'a' + 10;
	if ((hex >= 'A') && (hex <= 'F')) return hex - 'A' + 10;
	if ((hex >= '0') && (hex <= '9')) return hex - '0';
	return 15;
}

uint8_t si_setup=0;

// connectStatus - see "losttheplot" callback
// 1 means got WIFI
// 2 means initialised timers
// 4 means subscribed to MQTT
// 8 means WIFI has connected at least once since power up
// 16 means do above - if reset do nothing as unit is in wifi setup mode (see aidans_code.c)

uint32_t secondsUp;

int16_t rgbPlayBuffer[RGBMAX];
int16_t rgbPlayPtr = 0;
int16_t rgbRecordPtr = 0;
int8_t  rgbDoSeq = 0;
int16_t rgbTotal = 60;
int8_t rgbOnce = 0;

uint16_t out0Timer = 0;
uint16_t out2Timer = 0;
uint16_t out4Timer = 0;
uint16_t out5Timer = 0;
uint16_t out12Timer = 0;
uint16_t out13Timer = 0;
uint16_t out14Timer = 0;
uint16_t out15Timer = 0;
uint16_t out16Timer = 0;

#define INDOFF 10
// nothing

int8_t e13OnTime = 1;
int8_t e13OffTime = 10;
int8_t e13OnSet = 2;
int8_t e13OffSet = INDOFF;
int8_t pin14changed = 0;
int8_t pin2changed = 0;
uint8_t buzzer;

LOCAL void IFA ota_cb(void *arg)
{
	system_restart();
}

int16_t ICACHE_FLASH_ATTR roundDivision(int32_t numerator, int32_t denominator)
{
	int32_t result;

    if( ( (numerator > 0) && (denominator < 0) ) ||
        ( (numerator < 0) && (denominator > 0) ) )
    {
        result = ( numerator - (denominator/2) ) / denominator;
    }
    else
    {
        result = ( numerator + (denominator/2 )) / denominator;
    }
    return result;
}

static void IFA OtaUpdate_CallBack(bool result, uint8 rom_slot) {
	if (result == true) {
		// success
		if (rom_slot == FLASH_BY_ADDR) {
			iprintf(INFO, "Write Successful");
			setFlashBack(INDIC_B_SUCCEEDED_OTA);		// Doesn't like this ncherry@linuxha.com
		} else {
			// set to boot new rom and then reboot
			iprintf(INFO, "OTA succeeded to ROM %d", rom_slot);
			sysCfg.OTAdone = 'Y';
			cfgSave(); // store note that OTA is good
			setFlashBack(INDIC_B_SUCCEEDED_OTA);
			rboot_set_current_rom(rom_slot);
			os_timer_disarm(&otaTimer);
			os_timer_setfn(&otaTimer, (os_timer_func_t *) ota_cb, (void *) 0);
			os_timer_arm(&otaTimer, 2000, 0);
			//system_restart();
		}
	} else { // if result not true
		// fail
		iprintf(INFO, "Update failed\r\n");
		setFlashBack(INDIC_B_FAILED_OTA);
	}
}

static void IFA OtaUpdate()
{
	// start the upgrade process
	if (rboot_ota_start((ota_callback) OtaUpdate_CallBack))
	{
	iprintf(INFO, "Attempting update\r\n");
	setFlashBack(INDIC_B_ATTEMPTING_OTA);
	}
	else
	{
	iprintf(INFO, "Update failed\r\n");
	setFlashBack(INDIC_B_FAILED_OTA);
	}
}

static const uint8_t IRA kelvin[] = {
	255,  51,   0, //    1000
	255, 109,   0, //    1500
	255, 137,  18, //    2000
	255, 161,  72, //    2500
	255, 180, 107, //    3000
	255, 196, 137, //    3500
	255, 209, 163, //    4000
	255, 219, 186, //    4500
	255, 228, 206, //    5000
	255, 236, 224, //    5500
	255, 243, 239, //    6000
	255, 249, 253, //    6500
	245, 243, 255, //    7000
	235, 238, 255, //    7500
	227, 233, 255, //    8000
	220, 229, 255, //    8500
	214, 225, 255, //    9000
	208, 222, 255, //    9500
	204, 219, 255, //    10000
	200, 217, 255, //    10500
	196, 215, 255, //    11000
	193, 213, 255, //    11500
	191, 211, 255, //    12000
	188, 210, 255, //    12500
	186, 208, 255, //    13000
	184, 207, 255, //    13500
	182, 206, 255, //    14000
	180, 205, 255, //    14500
	179, 204, 255, //    15000
	177, 203, 255, //    15500
	176, 202, 255, //    16000
	175, 201, 255, //    16500
	174, 200, 255, //    17000
	173, 200, 255, //    17500
	172, 199, 255, //    18000
	171, 198, 255, //    18500
	170, 198, 255, //    19000
	169, 197, 255, //    19500
	168, 197, 255, //    20000
	168, 196, 255, //    20500
	167, 196, 255, //    21000
	166, 195, 255, //    21500
	166, 195, 255, //    22000
	165, 195, 255, //    22500
	164, 194, 255, //    23000
	164, 194, 255, //    23500
	163, 194, 255, //    24000
	163, 193, 255, //    24500
	163, 193, 255, //    25000
	162, 193, 255, //    25500
	162, 192, 255, //    26000
	161, 192, 255, //    26500
	161, 192, 255, //    27000
	161, 192, 255, //    27500
	160, 191, 255, //    28000
	160, 191, 255, //    28500
	160, 191, 255, //    29000
	159, 191, 255, //    29500
	159, 191, 255, //    30000
	159, 190, 255, //    30500
	159, 190, 255, //    31000
	158, 190, 255, //    31500
	158, 190, 255, //    32000
	158, 190, 255, //    32500
	158, 190, 255, //    33000
	157, 189, 255, //    33500
	157, 189, 255, //    34000
	157, 189, 255, //    34500
	157, 189, 255, //    35000
	157, 189, 255, //    35500
	156, 189, 255, //    36000
	156, 189, 255, //    36500
	156, 189, 255, //    37000
	156, 188, 255, //    37500
	156, 188, 255, //    38000
	155, 188, 255, //    38500
	155, 188, 255, //    39000
	155, 188, 255, //    39500
	155, 188, 255  //    40000
};

static const uint8_t IRA daft[8] ={
0x30,0x10,0x20,0x5F,
0x00,0x00,0x00,0x00
};

// This font can be freely used without any restriction(It is placed in public domain) - used for SEED display
static const uint8_t IRA BasicFont[] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00, //  ! lsb at top - bytes left to right
	0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x00, 0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00, 0x00,
	0x00, 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00, 0x00,
	0x00, 0x23, 0x13, 0x08, 0x64, 0x62, 0x00, 0x00,
	0x00, 0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x00,
	0x00, 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x41, 0x22, 0x1C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x2A, 0x1C, 0x2A, 0x08, 0x00, 0x00,
	0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00,
	0x00, 0xA0, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00,
	0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 0x00,
	0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x00, //0
	0x00, 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, 0x00,
	0x00, 0x62, 0x51, 0x49, 0x49, 0x46, 0x00, 0x00,
	0x00, 0x22, 0x41, 0x49, 0x49, 0x36, 0x00, 0x00,
	0x00, 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, 0x00,
	0x00, 0x27, 0x45, 0x45, 0x45, 0x39, 0x00, 0x00,
	0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00, 0x00,
	0x00, 0x01, 0x71, 0x09, 0x05, 0x03, 0x00, 0x00,
	0x00, 0x36, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00,
	0x00, 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x00,
	0x00, 0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xAC, 0x6C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00,
	0x00, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00,
	0x00, 0x41, 0x22, 0x14, 0x08, 0x00, 0x00, 0x00,
	0x00, 0x02, 0x01, 0x51, 0x09, 0x06, 0x00, 0x00,
	0x00, 0x32, 0x49, 0x79, 0x41, 0x3E, 0x00, 0x00,
	0x00, 0x7E, 0x09, 0x09, 0x09, 0x7E, 0x00, 0x00, // A
	0x00, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00,
	0x00, 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00, 0x00,
	0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00, 0x00,
	0x00, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00, 0x00,
	0x00, 0x7F, 0x09, 0x09, 0x09, 0x01, 0x00, 0x00,
	0x00, 0x3E, 0x41, 0x41, 0x51, 0x72, 0x00, 0x00,
	0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x00,
	0x00, 0x41, 0x7F, 0x41, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x20, 0x40, 0x41, 0x3F, 0x01, 0x00, 0x00,
	0x00, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00,
	0x00, 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00,
	0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00, 0x00,
	0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00, 0x00,
	0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, 0x00,
	0x00, 0x7F, 0x09, 0x09, 0x09, 0x06, 0x00, 0x00,
	0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00, 0x00,
	0x00, 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, 0x00,
	0x00, 0x26, 0x49, 0x49, 0x49, 0x32, 0x00, 0x00,
	0x00, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00, 0x00,
	0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x00,
	0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00, 0x00,
	0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00, 0x00,
	0x00, 0x63, 0x14, 0x08, 0x14, 0x63, 0x00, 0x00,
	0x00, 0x03, 0x04, 0x78, 0x04, 0x03, 0x00, 0x00,
	0x00, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x00,
	0x00, 0x7F, 0x41, 0x41, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x00,
	0x00, 0x41, 0x41, 0x7F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x04, 0x02, 0x01, 0x02, 0x04, 0x00, 0x00,
	0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00,
	0x00, 0x01, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x20, 0x54, 0x54, 0x54, 0x78, 0x00, 0x00,
	0x00, 0x7F, 0x48, 0x44, 0x44, 0x38, 0x00, 0x00,
	0x00, 0x38, 0x44, 0x44, 0x28, 0x00, 0x00, 0x00,
	0x00, 0x38, 0x44, 0x44, 0x48, 0x7F, 0x00, 0x00,
	0x00, 0x38, 0x54, 0x54, 0x54, 0x18, 0x00, 0x00,
	0x00, 0x08, 0x7E, 0x09, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C, 0x00, 0x00,
	0x00, 0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, 0x00,
	0x00, 0x00, 0x7D, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x84, 0x7D, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x7F, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00,
	0x00, 0x41, 0x7F, 0x40, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x7C, 0x04, 0x18, 0x04, 0x78, 0x00, 0x00,
	0x00, 0x7C, 0x08, 0x04, 0x7C, 0x00, 0x00, 0x00,
	0x00, 0x38, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00,
	0x00, 0xFC, 0x24, 0x24, 0x18, 0x00, 0x00, 0x00,
	0x00, 0x18, 0x24, 0x24, 0xFC, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7C, 0x08, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x48, 0x54, 0x54, 0x24, 0x00, 0x00, 0x00,
	0x00, 0x04, 0x7F, 0x44, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x3C, 0x40, 0x40, 0x7C, 0x00, 0x00, 0x00,
	0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00, 0x00,
	0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00, 0x00,
	0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00,
	0x00, 0x1C, 0xA0, 0xA0, 0x7C, 0x00, 0x00, 0x00,
	0x00, 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, 0x00,
	0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x41, 0x36, 0x08, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x02, 0x01, 0x01, 0x02, 0x01, 0x00, 0x00,
	0x00, 0x02, 0x05, 0x05, 0x02, 0x00, 0x00, 0x00
};

// this can be a uint8_t table if you take it out of FLASH.
static const uint8_t IRA ledTable[256] =
{	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4,
	4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 18,
	18, 19, 19, 20, 20, 21, 22, 22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 33, 33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41,
	42, 43, 44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 67, 68, 69, 70, 71, 72, 73, 75, 76, 77,
	78, 80, 81, 82, 83, 85, 86, 87, 89, 90, 91, 93, 94, 95, 97, 98, 99, 101, 102, 104, 105, 107, 108, 110, 111, 113, 114, 116, 117, 119, 121,
	122, 124, 125, 127, 129, 130, 132, 134, 135, 137, 139, 141, 142, 144, 146, 148, 150, 151, 153, 155, 157, 159, 161, 163, 165, 166, 168, 170,
	172, 174, 176, 178, 180, 182, 184, 186, 189, 191, 193, 195, 197, 199, 201, 204, 206, 208, 210, 212, 215, 217, 219, 221, 224, 226, 228, 231,
	233, 235, 238, 240, 243, 245, 248, 250, 253, 255
};

// this can be a uint16_t table if you take it out of FLASH.
static const uint32_t IRA PWMTable[100] =
{	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 24, 26, 28, 30, 34, 38, 42, 47, 54, 69, 84, 104, 124, 154, 184,
	224, 264, 314, 364, 439, 518, 618, 718, 868, 1108, 1308, 1508, 1758, 1950, 2150, 2350, 2550, 2770, 2950, 3200, 3500, 3800, 4100, 4400, 4700,
	5000, 5400, 5800, 6200, 6600, 7000, 7500, 8000, 8500, 9000, 9500, 10000, 10500, 11100, 11500, 12000, 12500, 13000, 13500, 14000, 14500, 15000, 15500,
	16000, 16500, 17000, 17500, 18000, 18500, 19000, 19500, 20000, 20500, 21000, 21500, 22000, 22500, 23000
};

uint16_t spiActive;

//
// This routine courtesy Richard A Burton - way better than using 32 bit flash arrays (you can't directly
// access 8 bit FLASH arrays - will crash the processor). Tried cacheing - made ZERO speed difference.
uint8 read_rom_uint8(const uint8* addr) {
	ETS_INTR_LOCK();
	uint32 bytes;
	bytes = *(uint32*)((uint32)addr & ~3);
	ETS_INTR_UNLOCK();
	return ((uint8*)&bytes)[(uint32)addr & 3];
}

//
// This routine developed from the above to read 16 bits from FLASH.
uint16 read_rom_uint16(const uint8* addr) {
	ETS_INTR_LOCK();
	uint32 bytes;
	bytes = *(uint32*)((uint32)addr & ~3);
	ETS_INTR_UNLOCK();
	//uint8_t *x;
	//x=&bytes;
	//if (((uint32_t)addr) & 2) return (uint16)(x[2]*256+x[3]); else return (uint16)(x[0]*256+x[1]);
	uint16_t *x;
	x = (uint16_t *) (&bytes);
	if (((uint32_t)addr) & 2) return (x[1]); else return (x[0]);
}

uint8_t in0Value=255;
uint8_t sonoffBounceCount=0;
uint8_t in0Toggle=0;
uint32_t lcd_parallel = 0;
uint8_t lcdBacklightStatus = 0;
uint8_t  hitachi_line_3, hitachi_line_4; uint8_t  hitachi_offset;
uint8_t newports[8];

// ----------------------------------------------------------------------------
// -- This WS2812 code must be compiled with -O2 to get the timing right.  Read this:
// -- http://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
// -- The IFA is there to trick the compiler and get the very first pulse width correct.
static void  __attribute__((optimize("O2"))) send_ws_0(uint8_t gpio) {
	uint8_t i;

	i = 4; while (i--) GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << gpio);
	i = 9; while (i--) GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1 << gpio);
}

static void __attribute__((optimize("O2"))) send_ws_1(uint8_t gpio) {
	uint8_t i;

	i = 8; while (i--) GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << gpio);
	i = 6; while (i--) GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1 << gpio);
}

// Some assumptions-  your offset is on a 4-byte boundary...(size doesn't have to be) - your sector is 0-253 (for the top 2 meg)
// and your offset + buffer does not go over the 4096 (remember - first 4 bytes used up.

uint8_t IFA my_flash_read(uint32_t fsec,uint32_t foff, uint32_t *fbuf,uint32_t fsiz)
{ //READ
uint32_t first;
uint32_t second;
uint32_t current;
uint32_t structsize;
uint32_t flashbase;
uint32_t flashoffs;
uint32_t tmp;
uint8_t good;

if (fsec>253) { return 0; } // assuming starting at third meg and can't touch very top 4 secs (2 for each of these)
if ((foff+4+fsiz)>4096) return 0;
uint32_t flashStart=(fsec*2)+0x200000;

// size of struct must be rounded up - offset from base=arg1, ram buffer = arg2,
// size=arg3
// when writing if length not 32-bit rounded, will write to nearest boundary up

flashbase=foff&0xfffff000;
flashoffs=foff&0xfff;

structsize=fsiz; if (structsize&3) structsize+=(4-(structsize&3));
current=4096;
spi_flash_read(flashStart+flashbase,&first,4);
spi_flash_read(flashStart+flashbase+current,&second,4);

if ((first==0xffffffff) && (second==0xffffffff)) // ie all new
	{
	good=0;
	spi_flash_erase_sector(0x200+(flashbase/4096));
	tmp=1;
	spi_flash_write(flashbase+flashStart,&tmp,4);
	spi_flash_read(flashbase+flashStart,&tmp,4);
    current=0;
	if (tmp==1) good=1; else good=0;
	}
else if (first==0xffffffff) current=1;
else if (second==0xffffffff) current=0;
else if (second>first) current=1;
else current=0;

// can't read whole in once go if struct not on 4 byte boundary

current*=4096;
if (structsize==fsiz)
	{
    spi_flash_read(flashoffs+flashStart+4+current,&first,4);
	if (first!=0xffffffff) good=1; else good=0;
	spi_flash_read(flashoffs+flashStart+4+current,fbuf,fsiz);
	}
	else
	{
    spi_flash_read(flashoffs+flashStart+4+current,&first,4);
	if (first!=0xffffffff) good=1; else good=0;
	spi_flash_read(flashoffs+flashStart+4+current,fbuf,(fsiz&0xfffffffc));
	spi_flash_read(flashoffs+flashStart+4+current+(fsiz&0xfffffffc),&tmp,4);    //// CHECK
	memcpy(fbuf+(fsiz&0xfffffffc),&tmp,(fsiz&3)); // move those last 1,3 or 3 bytes
	}

return good;  // so you know if it is the first time or not.
} // done with READ operation - phew!!

// this version of writing does NOT use a large buffer

uint8_t IFA my_flash_write(uint32_t fsec,uint32_t foff, uint32_t *fbuf,uint32_t fsiz)
{ //WRITE
uint32_t first;
uint32_t second;
uint32_t current;
uint32_t newcurrent;
uint32_t counter;
uint32_t tmp;
uint32_t flashbase;
uint32_t flashoffs;
uint32_t structsize;
uint8_t good;

if (fsec>253) { return 0; } // assuming starting at third meg and can't touch very top 4 secs (2 for each of these)
if ((foff+4+fsiz)>4096) return 0;

uint32_t flashStart=(fsec*2)+0x200000;

flashbase=foff&0xfffff000;
flashoffs=foff&0xfff;

// size of struct must be rounded up - offset from base=arg1, ram buffer = arg2,
// size=arg3
// when writing if length not 32-bit rounded, will write to nearest boundary up

structsize=fsiz; if (structsize&3) structsize+=(4-(structsize&3));
current=4096;
spi_flash_read(flashbase+flashStart,&first,4);
spi_flash_read(flashbase+flashStart+current,&second,4);

if ((first==0xffffffff) && (second==0xffffffff)) current=0;
else if (first==0xffffffff) current=1;
else if (second==0xffffffff) current=0;
else if (second>first) current=1;
else current=0;
	{
	good=0;
	spi_flash_erase_sector(0x200+(flashbase/4096)+(current^1)); // erase the OTHER one
	current *=4096;
	if (current) newcurrent=0; else newcurrent=4096;
	if (current) counter=second; else counter=first; if (counter==0xffffffff) counter++;

	tmp=0xffffffff;
	spi_flash_write(flashbase+flashStart+newcurrent,&tmp,4);	// write a blank counter

	uint32_t tstart,tstart2,tend;
	tstart=flashbase+flashStart+current+4;
	tstart2=flashbase+flashStart+newcurrent+4;
	tend=flashbase+flashStart+current+4+flashoffs;

	while (tstart<tend)
		{
			spi_flash_read(tstart,&tmp,4);
			spi_flash_write(tstart2,&tmp,4);
			tstart+=4; tstart2+=4;
		}

	spi_flash_write(tstart2,fbuf,structsize);
	tstart+=structsize; tstart2+=structsize;

	while (tstart<4096)
		{
			spi_flash_read(tstart,&tmp,4);
			spi_flash_write(tstart2,&tmp,4);
			tstart+=4; tstart2+=4;
		}

    counter++;
	spi_flash_write(flashbase+flashStart+newcurrent,&counter,4);
	spi_flash_read(flashbase+flashStart+newcurrent,&tmp,4);
    if (tmp==counter) good=1;
	}
return good;  // so you know if it is the first time or not.
} // done with WRITE operation

void IFA WS2812OutBuffer(uint8_t * buffer, uint16_t length, uint16_t repetition)
{
	uint16_t i;

	if (wsBitMask > 16) return;
	ets_intr_lock();
	for (i = 0; i < 2; i++)
		WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + 8, 1 << wsBitMask);
	while (repetition--)
	{
		for (i = 0; i < length; i++)
		{
			uint8_t mask = 0x80;
			uint8_t byte = buffer[i];
			while (mask)
			{
				(byte & mask) ? send_ws_1(wsBitMask) : send_ws_0(wsBitMask);
				mask >>= 1;
			}
		}
	}
	ets_intr_unlock();
}

void IFA rgblight(uint8_t ptr, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t t_wsBitMask;
	uint8_t bf[3];

	bf[0] = read_rom_uint8(ledTable + g);
	bf[1] = read_rom_uint8(ledTable + r);
	bf[2] = read_rom_uint8(ledTable + b);
	t_wsBitMask = wsBitMask;
	wsBitMask = ptr;
	WS2812OutBuffer(bf, 3, 1);
	wsBitMask = t_wsBitMask;
}

void IFA rgblights(uint8_t ptr, uint8_t r, uint8_t g, uint8_t b, uint16_t count)
{
	uint8_t t_wsBitMask;
	uint8_t bf[3];
	bf[0] = read_rom_uint8(ledTable + g);
	bf[1] = read_rom_uint8(ledTable + r);
	bf[2] = read_rom_uint8(ledTable + b);
	t_wsBitMask = wsBitMask;
	wsBitMask = ptr;
	WS2812OutBuffer(bf, 3, count);
	wsBitMask = t_wsBitMask;
}

void IFA setFlashBack(uint8_t r, uint8_t g, uint8_t b, uint8_t duration)
{
	flashQue[flashOut][0] = r;
	flashQue[flashOut][1] = g;
	flashQue[flashOut][2] = b;
	flashQue[flashOut][3] = duration;
	flashOut++; flashOut &= 15;
}

void IFA setFlash(uint8_t r, uint8_t g, uint8_t b)
{
	flashMainR = r;
	flashMainG = g;
	flashMainB = b;
}

//uint32 io_info[][3] = {
//  {PWM_1_OUT_IO_MUX, PWM_1_OUT_IO_FUNC, PWM_1_OUT_IO_NUM},
//  {PWM_3_OUT_IO_MUX, PWM_3_OUT_IO_FUNC, PWM_3_OUT_IO_NUM},
//  {PWM_4_OUT_IO_MUX, PWM_4_OUT_IO_FUNC, PWM_4_OUT_IO_NUM}
//};

uint8 dummy[] = {1, 2, 3, 4, 5, 6, 7, 8};
typedef struct
{
	uint16_t reda;
	uint16_t greena;
	uint16_t bluea;
	uint16_t red;
	uint16_t green;
	uint16_t blue;
} LEDX;
LEDX pwm_array;

typedef struct
{
	uint8_t device;
	uint16_t active;
	uint16_t activeCounter;
	uint16_t gap;
	uint16_t gapCounter;
	uint16_t repeats;
} MIST;
MIST mist;

void IFA mqttConnectedCb(uint32_t *args)
{
	char baseBuf[TBUFSIZE];
	setFlashBack(INDIC_B_GOTMQTT);
	connectStatus |= 4;
	MQTT_Client* client = (MQTT_Client*) args;
	iprintf(INFO, "MQTT Broker connected\r\nDevice ID is %s\r\n", sysCfg.base);
	MQTT_Subscribe(client, "toesp", 0);
	os_sprintf(baseBuf, "%s/toesp", sysCfg.base);
	MQTT_Subscribe(client, baseBuf, 0);
	os_sprintf(baseBuf, "{\"id\":\"%s\",\"desc\":\"%s\",\"attribute\":\"%s\",\"fail\":\"%d\"}",
	           sysCfg.base, sysCfg.description, sysCfg.attribute, sysCfg.lastFail); // changed to json
	MQTT_Publish(&mqttClient, "esplogon", baseBuf, os_strlen(baseBuf), 0, 0);

	if (sysCfg.OTAdone == 'Y')
	{
		sysCfg.OTAdone = 255;
		cfgSave(); // store note that OTA is good
		MQTT_Publish(&mqttClient, "otacomplete", sysCfg.base, os_strlen(sysCfg.base), 0, 0);
	}
}

void IFA mqttDisconnectedCb(uint32_t *args)
{
	setFlashBack(INDIC_B_LOSTMQTT);
	connectStatus &= (255 - 4); // note disconnection to prevent repeats
	MQTT_Client* client = (MQTT_Client*) args;
	iprintf(INFO, "MQTT Disconnected\r\n");
}

void IFA mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*) args;
	iprintf(INFO, "-->\r\n");
}

/************************* Partial Arduino time library with bits added ******/
typedef struct
{
	uint8_t Second;
	uint8_t Minute;
	uint8_t Hour;
	uint8_t Wday;     // day of week, Sunday is day 1
	uint8_t Day;
	uint8_t Month;
	uint16_t Year;
	unsigned long Valid;
} tm_t;

tm_t tm;

#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

static const uint8_t monthDays[] =
{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };     // API starts months from 1, this array starts from 0

void IFA convertTime()
{
	// given myrtc as time in Linux format break into time components
	// this is a more compact version of the C library localtime function
	// note that internally the year is offset from 1970 - compensated at the end
	uint8_t year;
	uint8_t month, monthLength;
	uint32_t time;
	unsigned long days;

	time = (uint32_t) myRtc;
	tm.Second = time % 60;
	time /= 60;     // now it is minutes
	tm.Minute = time % 60;
	time /= 60;     // now it is hours
	tm.Hour = time % 24;
	time /= 24;     // now it is days
	tm.Wday = ((time + 4) % 7) + 1;     // Sunday is day 1

	year = 0;
	days = 0;
	while ((unsigned) (days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
		year++;
	}
	tm.Year = year;     // year is offset from 1970

	days -= LEAP_YEAR(year) ? 366 : 365;
	time -= days;     // now it is days in this year, starting at 0

	days = 0;
	month = 0;
	monthLength = 0;
	for (month = 0; month < 12; month++) {
		if (month == 1) {     // february
			if (LEAP_YEAR(year)) {
				monthLength = 29;
			}
			else {
				monthLength = 28;
			}
		}
		else {
			monthLength = monthDays[month];
		}

		if (time >= monthLength) {
			time -= monthLength;
		}
		else {
			break;
		}
	}
	tm.Month = month + 1;     // jan is month 1
	tm.Day = time + 1;     // day of month
	tm.Year = tm.Year + 1970;
}

/* end time */

#define MAXTIMINGS 10000
#define BREAKTIME 20



int IFA readBME280()
{
sint32 xtemperature;
		    	uint32 xpressure, xhumidity;
		    	char buff[20];
		    	char req[100];

			    i2c_master_gpio_init();
			    if(BME280_Init(BME280_OS_T_16, BME280_OS_P_16, BME280_OS_H_16,
			  					BME280_FILTER_16, BME280_MODE_NORMAL, BME280_TSB_05))
			        return 0;

			    if (BME280_ReadAll(&xtemperature, &xpressure, &xhumidity))
						return 0;
			    		else
			    		{
			    		highTemperature=roundDivision(xtemperature,10);
			    		temperature=roundDivision(xtemperature,100);
			    		pressure=roundDivision((xpressure>>8),100);
			    		humidity=roundDivision(xhumidity,1024);
			    		return 1;
			            }
			    }

/*
   Start of Dallas code.

   BIG adaptation of Paul Stoffregen's One wire library to the ESP8266 and
   Necromant's Frankenstein firmware by Erland Lewin <erland@lewin.nu>

   Paul's original library site:
     http://www.pjrc.com/teensy/td_libs_OneWire.html

   See also http://playground.arduino.cc/Learning/OneWire

   Stripped down to bare minimum by Peter Scargill for single DS18B20 or DS18B20P integer read
*/

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return;

void IFA ds_reset(uint8_t port)
{
	uint8_t retries = 125;

	easygpio_pinMode(port, EASYGPIO_PULLUP, EASYGPIO_INPUT);
	// wait until the wire is high... just in case
	do {
		if (--retries == 0) return;
		os_delay_us(2);
	} while (!easygpio_inputGet(port));
	easygpio_outputEnable(port, 0);
	os_delay_us(480);
	easygpio_outputDisable(port);
	os_delay_us(480);
}

// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static IFA inline void write_bit(uint8_t port,int v)
{
	easygpio_outputEnable(port, 0);
	//easygpio_outputSet(gpioPin, 0);
	if (v) {
		os_delay_us(10);
		easygpio_outputSet(port, 1);
		os_delay_us(55);
	}
	else {
		os_delay_us(65);
		easygpio_outputSet(port, 1);
		os_delay_us(5);
	}
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static IFA inline int read_bit(uint8_t port)
{
	int r;

	easygpio_outputEnable(port, 0);
	os_delay_us(3);
	easygpio_outputDisable(port);
	os_delay_us(10);
	r = easygpio_inputGet(port);
	os_delay_us(53);
	return r;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
void IFA ds_write(uint8_t port,uint8_t v, int power)
{
	uint8_t bitMask;

	for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		write_bit(port, (bitMask & v) ? 1 : 0);
	}
	if (!power) {
		easygpio_outputDisable(port);
		easygpio_outputSet(port, 0);
	}
}

//
// Read a byte
//
uint8_t IFA ds_read(uint8_t port)
{
	uint8_t bitMask;
	uint8_t r = 0;

	for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		if (read_bit(port)) r |= bitMask;
	}
	return r;
}

int IFA readDallas(uint8_t port)
{
	int a;
	ds_reset(port);
	ds_write(port,0xcc, 1); // read dallas
	ds_write(port,0xbe, 1);
	a = (int) ds_read(port);
	a = a + (int) ds_read(port) * 256;
	a=a*10; a/=16;
	ds_reset(port);
	ds_write(port,0xcc, 1); // trigger a new read for next time
	ds_write(port,0x44, 1);
	if (a>1500) a-=40960;
	if (gotDsReading&(1<<port)==0) { a=-1; gotDsReading|=(1<<port); }
	return a;
}

// end of Dallas code

static void IFA readDHT(uint8_t port)
{
	int counter = 0;
	int laststate = 1;
	int i = 0;
	int j = 0;
	int checksum = 0;
	int data[100];

	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	easygpio_pinMode(port, EASYGPIO_PULLUP, EASYGPIO_OUTPUT);
	easygpio_outputSet(port, 1);
	os_delay_us(250000);
	easygpio_outputSet(port, 0);
	os_delay_us(20000);
	easygpio_outputSet(port, 1);
	os_delay_us(40);
	easygpio_outputDisable(port);

	// wait for pin to drop?
	while (easygpio_inputGet(port) == 1 && i < 100000) {
		os_delay_us(1);
		i++;
	}
	if (i == 100000) return;

	// read data!
	for (i = 0; i < MAXTIMINGS; i++) {
		counter = 0;
		while ( easygpio_inputGet(port) == laststate) {
			counter++;
			os_delay_us(1);
			if (counter == 1000) break;
		}
		laststate = easygpio_inputGet(port);
		if (counter == 1000) break;

		//bits[bitidx++] = counter;

		if ((i > 3) && (i % 2 == 0)) {
			// shove each bit into the storage bytes
			data[j / 8] <<= 1;
			if (counter > BREAKTIME) data[j / 8] |= 1;
			j++;
		}
	}

	float temp_p, hum_p;
	if (j >= 39) {
		checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
		if (data[4] == checksum) {
			/* yay! checksum is valid */
			if (sysCfg.sensor_type == 2)
			{
				hum_p = data[0];
				temp_p = data[2];
				highTemperature=temp_p;
			}
			else
			{
				hum_p = data[0] * 256 + data[1];
				hum_p /= 10;

				temp_p = (data[2] & 0x7F) * 256 + data[3];
				highTemperature=temp_p; if (data[2] & 0x80) highTemperature *= -1;
				temp_p /= 10;
				if (data[2] & 0x80) temp_p *= -1;
			}
			temperature = temp_p;
			humidity = hum_p;
		}
	}
}

void IFA hitachiByte(uint8_t data, uint8_t flag)
{
	if (lcd_parallel)
	{
		easygpio_pinMode(16, EASYGPIO_NOPULL, EASYGPIO_OUTPUT); // just to be sure
		sysCfg.enable13 = 1; // just to be sure
		// given gpio 5 is data control - and 4 is en - and 12,13,15,16 are D4-7.
		if (data & 16) easygpio_outputSet(12, 1); else easygpio_outputSet(12, 0);
		if (data & 32) easygpio_outputSet(13, 1); else easygpio_outputSet(13, 0);
		if (data & 64) easygpio_outputSet(15, 1); else easygpio_outputSet(15, 0);
		if (data & 128) easygpio_outputSet(16, 1); else easygpio_outputSet(16, 0);
		if (flag & 1) easygpio_outputSet(5, 1); else easygpio_outputSet(5, 0);
		easygpio_outputSet(4, 1); os_delay_us(5); easygpio_outputSet(4, 0);
		os_delay_us(35);
	}
	else
	{
		flag &= 1;
		flag |= lcdBacklightStatus;
		i2c_master_writeByte(data | 4 | (flag));
		i2c_master_checkAck();
		i2c_master_writeByte(data | (flag));
		i2c_master_checkAck();
	}
}

void IFA hitachiHiLo(uint8_t data, uint8_t flag)
{
	hitachiByte(data & 0xf0, flag);
	hitachiByte(data << 4, flag);
}

void IFA hitachiHiLoS(uint8_t *data, uint8_t flag)
{
	while (*data) hitachiHiLo(*data++, flag);
}

// PWM with PCA9685 start here
// Modified and simplified from this Adafruit lib for Arduino
// https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library/blob/master/Adafruit_PWMServoDriver.cpp

uint8_t PWMAddr=0x40;
#define PCA9685_MODE1 0x0
#define PCA9685_PRESCALE 0xFE

#define LED0_ON_L 0x6
#define LED0_ON_H 0x7
#define LED0_OFF_L 0x8
#define LED0_OFF_H 0x9

#define ALLLED_ON_L 0xFA
#define ALLLED_ON_H 0xFB
#define ALLLED_OFF_L 0xFC
#define ALLLED_OFF_H 0xFD

void IFA pwmWrite(unsigned char addr,unsigned char d)
{
	i2c_writeData(PWMAddr,addr,&d,1);
}

uint8_t IFA pwmRead(uint8_t addr)
{
	uint8_t a;
	i2c_readData(PWMAddr,addr,&a,1);
	return a;
}


int IFA ifloor(float x) {
    int xi = (int)x;
    return x < xi ? xi - 1 : xi;
}

void IFA pwmFrequency(uint8_t chipAddr, float freq)
{
	  PWMAddr=chipAddr;
	  pwmWrite(PCA9685_MODE1,0); // saves a reset function
	  freq *= 0.9;  // Correct for overshoot in the frequency setting
	  float prescaleval = 25000000;
	  prescaleval /= 4096;
	  prescaleval /= freq;
	  prescaleval -= 1;
	  uint8_t prescale = ifloor(prescaleval + 0.5);
	  uint8_t oldmode = pwmRead(PCA9685_MODE1);
	  uint8_t newmode = (oldmode&0x7F) | 0x10; // sleep
	  pwmWrite(PCA9685_MODE1, newmode); // go to sleep
	  pwmWrite(PCA9685_PRESCALE, prescale); // set the prescaler
	  pwmWrite(PCA9685_MODE1, oldmode);
	  os_delay_us(5000);
	  pwmWrite(PCA9685_MODE1, oldmode | 0xa1);  //  This sets the MODE1 register to turn on auto increment.
}

void IFA pwmSet(uint8_t chipAddr,uint8_t num, uint16_t on, uint16_t off)
{
	uint8_t pwset[5];
	pwset[0]=LED0_ON_L+4*num;
	pwset[1]=on;
	pwset[2]=on>>8;
	pwset[3]=off;
	pwset[4]=off>>8;
	i2c_writeData(chipAddr,pwset[0],&pwset[1],4);
}
// PWM ends here

//SEEED experiments start here

#define VERTICAL_MODE                       01
#define HORIZONTAL_MODE                     02

#define SeeedGreyOLED_Address               0x3c
#define SeeedGreyOLED_Command_Mode          0x80
#define SeeedGreyOLED_Data_Mode             0x40

#define SeeedGreyOLED_Display_Off_Cmd       0xAE
#define SeeedGreyOLED_Display_On_Cmd        0xAF

#define SeeedGreyOLED_Normal_Display_Cmd    0xA4
#define SeeedGreyOLED_Inverse_Display_Cmd   0xA7
#define SeeedGreyOLED_Activate_Scroll_Cmd   0x2F
#define SeeedGreyOLED_Dectivate_Scroll_Cmd  0x2E
#define SeeedGreyOLED_Set_ContrastLevel_Cmd 0x81

#define Scroll_Left             0x00
#define Scroll_Right            0x01

#define Scroll_2Frames          0x7
#define Scroll_3Frames          0x4
#define Scroll_4Frames          0x5
#define Scroll_5Frames          0x0
#define Scroll_25Frames         0x6
#define Scroll_64Frames         0x1
#define Scroll_128Frames        0x2
#define Scroll_256Frames        0x3

char addressingMode;

void seeedInit(void);

void seeedSetNormalDisplay();
void seeedSetInverseDisplay();

void seeedSendCommand(unsigned char command);
void seeedSendData(unsigned char Data);
void seeedSetGreyLevel(unsigned char greyLevel);

void seeedSetVerticalMode();
void seeedSetHorizontalMode();

void seeedSetTextXY(unsigned char Row, unsigned char Column);
void seeedClearDisplay();
void seeedSetContrastLevel(unsigned char ContrastLevel);
void seeedPutChar(unsigned char c);
void seeedPutString(const char *String);
unsigned char seeedPutNumber(long n);
//unsigned char seeedPutFloat(float floatNumber,unsigned char decimal);
//unsigned char seeedPutFloat(float floatNumber);

//void seeedDrawBitmap(const unsigned char *bitmaparray,int bytes);

void seeedSetHorizontalScrollProperties(bool direction, unsigned char startRow, unsigned char endRow, unsigned char startColumn, unsigned char endColumn, unsigned char scrollSpeed);
void seeedActivateScroll();
void seeedDeactivateScroll();

unsigned char greyH;
unsigned char greyL;

void IFA seeedSendCommand(unsigned char command)
{
	i2c_writeData(SeeedGreyOLED_Address,SeeedGreyOLED_Command_Mode,&command,1);
}

void IFA seeedSendData(unsigned char Data)
{
	i2c_writeData(SeeedGreyOLED_Address,SeeedGreyOLED_Data_Mode,&Data,1);
}

void IFA seeedSendClear(unsigned char data, uint16_t len)
{
	i2c_writeSameData(SeeedGreyOLED_Address,SeeedGreyOLED_Data_Mode,&data,len);
}

void IFA seeedInit(void)
{
	i2c_master_gpio_init();
	seeedSendCommand(0xFD); // Unlock OLED driver IC MCU interface from entering command. i.e: Accept commands
	seeedSendCommand(0x12);
	seeedSendCommand(0xAE); // Set display off
	seeedSendCommand(0xA8); // set multiplex ratio
	seeedSendCommand(0x5F); // 96
	seeedSendCommand(0xA1); // set display start line
	seeedSendCommand(0x00);
	seeedSendCommand(0xA2); // set display offset
	seeedSendCommand(0x60);
	seeedSendCommand(0xA0); // set remap
	seeedSendCommand(0x46);
	seeedSendCommand(0xAB); // set vdd internal
	seeedSendCommand(0x01); //
	seeedSendCommand(0x81); // set contrasr
	seeedSendCommand(0x53); // 100 nit
	seeedSendCommand(0xB1); // Set Phase Length
	seeedSendCommand(0X51); //
	seeedSendCommand(0xB3); // Set Display Clock Divide Ratio/Oscillator Frequency
	seeedSendCommand(0x01);
	seeedSendCommand(0xB9); //
	seeedSendCommand(0xBC); // set pre_charge voltage/VCOMH
	seeedSendCommand(0x08); // (0x08);
	seeedSendCommand(0xBE); // set VCOMH
	seeedSendCommand(0X07); // (0x07);
	seeedSendCommand(0xB6); // Set second pre-charge period
	seeedSendCommand(0x01); //
	seeedSendCommand(0xD5); // enable second precharge and enternal vsl
	seeedSendCommand(0X62); // (0x62);
	seeedSendCommand(0xA4); // Set Normal Display Mode
	seeedSendCommand(0x2E); // Deactivate Scroll
	seeedSendCommand(0xAF); // Switch on display
	os_delay_us(100000);
	// Row Address
	seeedSendCommand(0x75);    // Set Row Address
	seeedSendCommand(0x00);    // Start 0
	seeedSendCommand(0x5f);    // End 95

	// Column Address
	seeedSendCommand(0x15);    // Set Column Address
	seeedSendCommand(0x08);    // Start from 8th Column of driver IC. This is 0th Column for OLED
	seeedSendCommand(0x37);    // End at  (8 + 47)th column. Each Column has 2 pixels(segments)

	// Init gray level for text. Default:Brightest White
	uint8_t greyH = 0xF0;
	uint8_t greyL = 0x0F;
}

void IFA seeedSetContrastLevel(unsigned char ContrastLevel)
{
	seeedSendCommand(SeeedGreyOLED_Set_ContrastLevel_Cmd);
	seeedSendCommand(ContrastLevel);
}

void IFA seeedSetHorizontalMode()
{
	seeedSendCommand(0xA0); // remap to
	seeedSendCommand(0x42); // horizontal mode

	// Row Address
	seeedSendCommand(0x75);    // Set Row Address
	seeedSendCommand(0x00);    // Start 0
	seeedSendCommand(0x5f);    // End 95

	// Column Address
	seeedSendCommand(0x15);    // Set Column Address
	seeedSendCommand(0x08);    // Start from 8th Column of driver IC. This is 0th Column for OLED
	seeedSendCommand(0x37);    // End at  (8 + 47)th column. Each Column has 2 pixels(or segments)
}

void IFA seeedSetVerticalMode()
{
	seeedSendCommand(0xA0); // remap to
	seeedSendCommand(0x46); // Vertical mode
}

void IFA seeedSetTextXY(unsigned char Row, unsigned char Column)
{
	//Column Address
	seeedSendCommand(0x15);             /* Set Column Address */
	seeedSendCommand(0x08 + (Column * 4)); /* Start Column: Start from 8 */
	seeedSendCommand(0x37);             /* End Column */
	// Row Address
	seeedSendCommand(0x75);             /* Set Row Address */
	seeedSendCommand(0x00 + (Row * 8)); /* Start Row*/
	seeedSendCommand(0x07 + (Row * 8)); /* End Row*/
}

void IFA seeedClearDisplay()
{
	unsigned char i, j;
	for (j = 0; j < 48; j++)
	{
		for (i = 0; i < 96; i++) //clear all columns
		{
			seeedSendData(0x00);
		}
	}
}

void IFA seeedSetGreyLevel(unsigned char greyLevel)
{
	greyH = (greyLevel << 4) & 0xF0;
	greyL =  greyLevel & 0x0F;
}

void IFA seeedPutChar(unsigned char C)
{
	if (C < 32 || C > 127) //Ignore non-printable ASCII characters. This can be modified for multilingual font.
	{
		C = ' '; //Space
	}


	char i,j;
	for (i = 0; i < 8; i = i + 2)
	{
		for (j = 0; j < 8; j++)
		{
			// Character is constructed two pixel at a time using vertical mode from the default 8x8 font
			char c = 0x00;

			char bit1 = (read_rom_uint8(BasicFont + (((C - 32) * 8) + i)) >> j)  & 0x01;
			char bit2 = (read_rom_uint8(BasicFont + (((C - 32) * 8) + (i + 1))) >> j)  & 0x01;

			// Each bit is changed to a nibble
			c |= (bit1) ? greyH : 0x00;
			c |= (bit2) ? greyL : 0x00;
			seeedSendData(c);
		}
	}
}

void IFA seeedPutString(const char *String)
{
	unsigned char i = 0;
	while (String[i])
	{
		seeedPutChar(String[i]);
		i++;
	}
}

unsigned char IFA seeedPutNumber(long long_num)
{
	unsigned char char_buffer[10] = "";
	unsigned char i = 0;
	unsigned char f = 0;

	if (long_num < 0)
	{
		f = 1;
		seeedPutChar('-');
		long_num = -long_num;
	}
	else if (long_num == 0)
	{
		f = 1;
		seeedPutChar('0');
		return f;
	}

	while (long_num > 0)
	{
		char_buffer[i++] = long_num % 10;
		long_num /= 10;
	}

	f = f + i;
	for (; i > 0; i--)
	{
		seeedPutChar('0' + char_buffer[i - 1]);
	}
	return f;

}

void IFA seeedSetHorizontalScrollProperties(bool direction, unsigned char startRow, unsigned char endRow, unsigned char startColumn, unsigned char endColumn, unsigned char scrollSpeed)
{
	/*
	Use the following defines for 'direction' :
	Scroll_Left
	Scroll_Right
	Use the following defines for 'scrollSpeed' :
	Scroll_2Frames
	Scroll_3Frames
	Scroll_4Frames
	Scroll_5Frames
	Scroll_25Frames
	Scroll_64Frames
	Scroll_128Frames
	Scroll_256Frames
	*/

	if (Scroll_Right == direction)
	{
		//Scroll Right
		seeedSendCommand(0x27);
	}
	else
	{
		//Scroll Left
		seeedSendCommand(0x26);
	}
	seeedSendCommand(0x00);       //Dummmy byte
	seeedSendCommand(startRow);
	seeedSendCommand(scrollSpeed);
	seeedSendCommand(endRow);
	seeedSendCommand(startColumn + 8);
	seeedSendCommand(endColumn + 8);
	seeedSendCommand(0x00);      //Dummmy byte
}

void seeedActivateScroll()
{
	seeedSendCommand(SeeedGreyOLED_Activate_Scroll_Cmd);
}

void seeedDeactivateScroll()
{
	seeedSendCommand(SeeedGreyOLED_Dectivate_Scroll_Cmd);
}

void seeedSetNormalDisplay()
{
	seeedSendCommand(SeeedGreyOLED_Normal_Display_Cmd);
}

void seeedSetInverseDisplay()
{
	seeedSendCommand(SeeedGreyOLED_Inverse_Display_Cmd);
}

// end of experiments

void IFA ok()
{
	iprintf(RESPONSE, "Ok\r\n");
}

// HSV to RGB
struct RGB_set {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} RGB_set;

struct HSV_set {
	signed int h;
	unsigned char s;
	unsigned char v;
} HSV_set;

int inline fasterfloor( const float x ) {
	return x < 0 ? (int) x == x ? (int) x : (int) x - 1 : (int) x;
}
/*******************************************************************************
   Function HSV2RGB
   Description: Converts an HSV color value into its equivalen in the RGB color space.
   Copyright 2010 by George Ruinelli
   The code I used as a source is from http://www.cs.rit.edu/~ncs/color/t_convert.html
   Parameters:
     1. struct with HSV color (source)
     2. pointer to struct RGB color (target)
   Notes:
     - r, g, b values are from 0..255
     - h = [0,360], s = [0,255], v = [0,255]
     - NB: if s == 0, then h = 0 (undefined)
 ******************************************************************************/
void IFA HSV2RGB(struct HSV_set HSV, struct RGB_set *RGB) {
	int i;
	float f, p, q, t, h, s, v;

	h = (float)HSV.h;
	s = (float)HSV.s;
	v = (float)HSV.v;

	s /= 255;

	if ( s == 0 ) { // achromatic (grey)
		RGB->r = RGB->g = RGB->b = v;
		return;
	}

	h /= 60;            // sector 0 to 5
	//i = floor( h );
	i = fasterfloor(h);
	f = h - i;            // factorial part of h
	p = (unsigned char)(v * ( 1 - s ));
	q = (unsigned char)(v * ( 1 - s * f ));
	t = (unsigned char)(v * ( 1 - s * ( 1 - f ) ));

	switch ( i ) {
	case 0:
		RGB->r = v;
		RGB->g = t;
		RGB->b = p;
		break;
	case 1:
		RGB->r = q;
		RGB->g = v;
		RGB->b = p;
		break;
	case 2:
		RGB->r = p;
		RGB->g = v;
		RGB->b = t;
		break;
	case 3:
		RGB->r = p;
		RGB->g = q;
		RGB->b = v;
		break;
	case 4:
		RGB->r = t;
		RGB->g = p;
		RGB->b = v;
		break;
	default: // case 5:
		RGB->r = v;
		RGB->g = p;
		RGB->b = q;
		break;
	}
}

/*******************************************************************************
   Function RGB_string2RGB_struct
   Description: Converts an RGB color tring into the individual color values.
   Copyright 2010 by George Ruinelli
   Parameters:
     1. pointer string with RGB color (source) without leading # i.e. FF0000 for red.
     2. pointer tu struct RGB color struct (target)
   Notes:
     - r, g, b values are from 0..255
 ******************************************************************************/
void IFA RGB_string2RGB_struct(char *tuple, struct RGB_set *RGB) {
	char tmp[3];

	strncpy(tmp, tuple, 2); tmp[2] = '\0';
	RGB->r = (unsigned char)(strtoul(tmp, NULL, 16));
	strncpy(tmp, &tuple[2], 2); tmp[2] = '\0';
	RGB->g = (unsigned char)(strtoul(tmp, NULL, 16));
	strncpy(tmp, &tuple[4], 2); tmp[2] = '\0';
	RGB->b = (unsigned char)(strtoul(tmp, NULL, 16));
}

// So this code goes off and does a background scan for WIFI and outputs them for me

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

// I2c function to write to a second expander for more ports
void IFA morePorts(int port,int thebyte,int thebit, int newval)
{
	i2c_master_gpio_init();
	if (newval) newports[thebyte]|=(1<<thebit); else newports[thebyte]&=(255-(1<<thebit));
	i2c_writeData(port,newports[thebyte],NULL,0);
}

// I2c function to write to a second expander for more ports
int IFA moreInPorts(int port,int thebyte, int thebit)
{
	i2c_master_gpio_init();
	char l;
	i2c_readData(port,-1,&l,1);
	newports[thebyte]=l;
	if (l&(1<<thebit)) return 1; else return 0;
}


// Input a value 0 to 255 to get a colour value.
LOCAL IFA void colourWheel(uint8_t WheelPos)
{
	if (WheelPos < 85) {
		rgb.reda = (uint8_t) (WheelPos * 3);
		rgb.greena = (uint8_t) (255 - WheelPos * 3);
		rgb.bluea = 0;
	}
	else if (WheelPos < 170) {
		WheelPos -= 85;
		rgb.reda = (uint8_t) (255 - WheelPos * 3);
		rgb.greena = 0;
		rgb.bluea = (uint8_t) (WheelPos * 3);

	}
	else {
		WheelPos -= 170;
		rgb.reda = 0;
		rgb.greena = (uint8_t) (WheelPos * 3);
		rgb.bluea = (uint8_t) (255 - WheelPos * 3);
	}
}

LOCAL void IFA startup_cb(void *arg)
{
	if ((sysCfg.set2Out != 1) || (sysCfg.wifi_button !=2))
	{
		startupTimeout++;
		if (!easygpio_inputGet(sysCfg.wifi_button)) {
			sysCfg.wifiSetup += 1;
			iprintf(INFO, "+\r\n");
			if (sysCfg.wifiSetup > 4)
			{
				setFlashBack(INDIC_B_WEB_BUTTON_PRESSED);  e13OffSet = INDOFF;
			}
		}
		if (startupTimeout > 14) {
			if ((easygpio_inputGet(sysCfg.wifi_button)) && (sysCfg.wifiSetup == 0)) os_timer_disarm(&startupTimer);
			if (sysCfg.wifiSetup > 4)
			{
				if (easygpio_inputGet(sysCfg.wifi_button)) {
					sysCfg.wifiSetup = 1;
					cfgSave();
					system_restart();
				}
			}
			else sysCfg.wifiSetup = 0;
		}
	}
}

LOCAL void IFA led_cb(void *arg)
{
	if ((pwm_array.reda != pwm_array.red) || (pwm_array.greena != pwm_array.green) || (pwm_array.bluea != pwm_array.blue))
	{
		if (pwm_array.reda > pwm_array.red) pwm_array.reda--;
		if (pwm_array.reda < pwm_array.red) pwm_array.reda++;
		if (pwm_array.greena > pwm_array.green) pwm_array.greena--;
		if (pwm_array.greena < pwm_array.green) pwm_array.greena++;
		if (pwm_array.bluea > pwm_array.blue) pwm_array.bluea--;
		if (pwm_array.bluea < pwm_array.blue) pwm_array.bluea++;
		if (donepwm == 0) {
			pwm_init(freq, duty, 3, io_info);
			donepwm = 1;
		}
		pwm_set_duty((uint32_t)PWMTable[pwm_array.reda], (uint8)0);
		pwm_set_duty((uint32_t)PWMTable[pwm_array.greena], (uint8)1);
		pwm_set_duty((uint32_t)PWMTable[pwm_array.bluea], (uint8)2);
		pwm_start();
	}

	//Handle Serial LEDS

	if (rgb.rainbow) {

		rgb.rainbow--;
		if (rgb.rainbow == 0) {
			rgb.red = 0;
			rgb.green = 0;
			rgb.blue = 0;
			rgb.rgbdelay = 5;
		}
		else {
			colourWheel((uint8_t) (rgb.rainbow & 255));
			rgb.buffer[0] = rgb.reda;
			rgb.buffer[1] = rgb.greena;
			rgb.buffer[2] = rgb.bluea;
			WS2812OutBuffer(rgb.buffer, 3, rgb.rgbnum);     // 3 leds in array, number of repetitions
		}
	}

	else
		/// rgb to fade from any colour to any other colour for any number of LEDS for any given period in secs
		if ((rgb.red != rgb.reda) || (rgb.green != rgb.greena) || (rgb.blue != rgb.bluea) || (rgbOnce)) {
			rgbOnce = 0;
			if (rgb.reda < rgb.red) rgb.reda += ((rgb.red - rgb.reda) / (rgb.rgbdelay * 20)) + 1;
			if (rgb.greena < rgb.green) rgb.greena += ((rgb.green - rgb.greena) / (rgb.rgbdelay * 20)) + 1;
			if (rgb.bluea < rgb.blue) rgb.bluea += ((rgb.blue - rgb.bluea) / (rgb.rgbdelay * 20)) + 1;
			if (rgb.reda > rgb.red) rgb.reda -= ((rgb.reda - rgb.red) / (rgb.rgbdelay * 20)) + 1;
			if (rgb.greena > rgb.green) rgb.greena -= ((rgb.greena - rgb.green) / (rgb.rgbdelay * 20)) + 1;
			if (rgb.bluea > rgb.blue) rgb.bluea -= ((rgb.bluea - rgb.blue) / (rgb.rgbdelay * 20)) + 1;

			if (rgb.rgbnum == 0) {
				rgb.rgbnum = 1;
				rgb.reda = rgb.red;
				rgb.greena = rgb.green;
				rgb.bluea = rgb.blue;
			}     // instant

			rgb.buffer[0] = read_rom_uint8(ledTable + rgb.reda);
			rgb.buffer[1] = read_rom_uint8(ledTable + rgb.greena);
			rgb.buffer[2] = read_rom_uint8(ledTable + rgb.bluea);


			WS2812OutBuffer(rgb.buffer, 3, rgb.rgbnum);     // 3 leds in array, number of repetitions
		}
}

LOCAL void IFA flash_cb(void *arg)
{
	// flashing light

	if (sysCfg.enable13 == 0) { // only if 13 not used elsewhere


		if (state13 == 1) {
			if (e13OnTime)
			{
				if (--e13OnTime == 0)
				{
					e13OnTime = e13OnSet;
					if (sysCfg.rgb_ind) {
						if (flashIn == flashOut) { rgblight(13, 0, 0, 0); e13OffSet = NORMAL_LED_OFF_TIME;  e13OffTime = e13OffSet; }
						else
						{
							rgblight(13, flashQue[flashIn][0], flashQue[flashIn][1], flashQue[flashIn][2]); e13OffSet = flashQue[flashIn][3];
							flashIn++; flashIn &= 15;
						}
					}
					else { if (sysCfg.electrodragon) easygpio_outputSet(16, OUT_OFF ^ sysCfg.sonoff); else easygpio_outputSet(13, OUT_OFF ^ sysCfg.sonoff); }
					state13 = 0;
				}
			}
		}
		else {
			if (e13OffTime)
			{
				if (--e13OffTime == 0)
				{
					e13OffTime = e13OffSet;
					if (sysCfg.rgb_ind) { rgblight(13, flashMainR, flashMainG, flashMainB); }
					else { if (sysCfg.electrodragon) easygpio_outputSet(16, OUT_ON ^ sysCfg.sonoff); else easygpio_outputSet(13, OUT_ON ^ sysCfg.sonoff); }
					state13 = 1;
				}
			}
		}

	}
}

LOCAL void IFA serial_cb(void *arg)
{
	if  (serialBCount) {
		uint8_t tOffs;
		tOffs = 0;
		while (serialInBuf[serialOutOffset]) serialTBuf[tOffs++] = serialInBuf[serialOutOffset++];
		serialTBuf[tOffs++] = serialInBuf[serialOutOffset++];
		mqttDataCb(NULL, "serial$", 7, serialTBuf, os_strlen(serialTBuf));
		--serialBCount;
	}
}

//*** Bounce timer. This is 25ms period.
LOCAL void IFA bounce_cb(void *arg)
{
	char tBuf[84];
	char pBuf[84];

	// read Nextion data at 56k
	char nextion_buffer[96];
	if (Softuart_Readline_Nextion(&softuart2, nextion_buffer, 95) > 0)
	{
		//iprintf(RESPONSE,nextion_buffer+1);
		char *np;
		char ncount;

		ncount = 0;
		np = nextion_buffer + 1;
		while ((ncount < 83) && (*np) && (*np != '~')) {
			tBuf[ncount++] = *np++;
			tBuf[ncount] = 0;
		}
		if (*np == '~')
		{
			ncount = 0; np++;
			while ((ncount < 83) && (*np)) {
				pBuf[ncount++] = *np++;
				pBuf[ncount] = 0;
			}
			if (*np == 0)
			{
				easygpio_outputSet(12, 1);
				buzzer = 8;
				MQTT_Publish(&mqttClient, tBuf, pBuf, os_strlen(pBuf), 0, 0);
			}
		}
	}

	if (buzzer) {
		if (--buzzer == 0) easygpio_outputSet(12, 0);
	}

	if (sysCfg.temperaturePort != 2)
	{
		if (in2BounceCount >= sysCfg.in2Bounce) {
			in2BounceCount = 0;
			in2Value = easygpio_inputGet(2);
			// send new value...

			os_sprintf(tBuf, "%s/fromesp/trigger2", sysCfg.base);
			if ((in2Value) && (pin2changed == -1)) MQTT_Publish(&mqttClient, tBuf, "1", 1, 0, 0);
			if ((in2Value == 0) && (pin2changed == 1)) MQTT_Publish(&mqttClient, tBuf, "0", 1, 0, 0);

			if (in2Value == 0) {
				in2Count++;
				pin2changed = -1;
			}
			else {
				pin2changed = 1;
			}
		}
		if (in2Value != easygpio_inputGet(2)) in2BounceCount++;
		else in2BounceCount = 0;
	}

	if ((sysCfg.temperaturePort != 14) && (spiActive == 0) &&(sysCfg.out14Enable==0))
	{
		if (inBounceCount >= sysCfg.in14Bounce) {
			inBounceCount = 0;
			in14Value = easygpio_inputGet(14);
			// send new value...

			os_sprintf(tBuf, "%s/fromesp/trigger14", sysCfg.base);
			if ((in14Value) && (pin14changed == -1)) MQTT_Publish(&mqttClient, tBuf, "1", 1, 0, 0);
			if ((in14Value == 0) && (pin14changed == 1)) MQTT_Publish(&mqttClient, tBuf, "0", 1, 0, 0);

			if (in14Value == 0) {
				in14Count++;
				pin14changed = -1;
			}
			else {
				pin14changed = 1;
			}
		}
		if (in14Value != easygpio_inputGet(14)) inBounceCount++;
		else inBounceCount = 0;
	}



	if ((sysCfg.sonoff)&&(sysCfg.wifi_button == 0)) // if in Sonoff mode and gpio0 is an input.... use gpio to toggle port 12
			{
			if (sonoffBounceCount >= 6)
				{
				sonoffBounceCount = 0;
				in0Value = easygpio_inputGet(0);
				if ((sysCfg.out12Status != 6)&&(in0Value == 0))
									{
										in0Toggle^=1;
										(sysCfg.invert & 16) ? easygpio_outputSet(12, ((in0Toggle&1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(12,((in0Toggle&1) ? OUT_ON : OUT_OFF));
										if (in0Toggle) sysCfg.out12Status=1; else sysCfg.out12Status=0;
										cfgSave();
									}
				}
			if (in0Value != easygpio_inputGet(0)) sonoffBounceCount++;
			else sonoffBounceCount = 0;
			}

	// now check for manual override on output 0
	if (sysCfg.override0) {
		if (pin14changed == 1) {
			pinChangeDownCounter = sysCfg.override0Time;
			easygpio_outputSet(sysCfg.relayoutput_override, 1);
		}     // overwrite output 0 with button
		if (pin14changed == -1) {
			pinChangeDownCounter = sysCfg.override0Time;
			easygpio_outputSet(sysCfg.relayoutput_override, 0);
		}
	}
	if (pinChangeDownCounter) {
		if (--pinChangeDownCounter == 0) easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.out0Status == 1) ? OUT_ON : OUT_OFF));
	}
}

LOCAL void IFA temperature_cb(void *arg)
{
	char tBuf[84];
	char pBuf[12];
	if (sysCfg.temperaturePort)  // could be 2 or 14 - or nothing...
	{
		if ((sysCfg.sensor_type == 1) || (sysCfg.sensor_type == 2))
		{
			readDHT(sysCfg.temperaturePort);
			pressure=0;
			os_sprintf(tBuf, "%s/fromesp/auto_temperature", sysCfg.base);
			os_sprintf(pBuf, "%d", temperature);
			MQTT_Publish(&mqttClient, tBuf, pBuf, os_strlen(pBuf), 0, 0);
			os_sprintf(tBuf, "%s/fromesp/auto_humidity", sysCfg.base);
			os_sprintf(pBuf, "%d", humidity);
			MQTT_Publish(&mqttClient, tBuf, pBuf, os_strlen(pBuf), 0, 0);
		}
		else if (sysCfg.sensor_type == 0) {
			int a=readDallas(sysCfg.temperaturePort);
			if (a==-1)
				{
				humidity = 0;
				pressure = 0;
				highTemperature=0;
				temperature = 0;
				}
			else
				{
				humidity = 0;
				pressure = 0;
				highTemperature=a;
				temperature=a/10;
				os_sprintf(tBuf, "%s/fromesp/auto_temperature", sysCfg.base);
				os_sprintf(pBuf, "%d", temperature);
				MQTT_Publish(&mqttClient, tBuf, pBuf, os_strlen(pBuf), 0, 0);
				}
		}
		else if (sysCfg.sensor_type==3)
			{
		    if (readBME280())
				{
		    	os_sprintf(tBuf, "%s/fromesp/auto_temperature", sysCfg.base);
		    	os_sprintf(pBuf, "%d", temperature);
		    	MQTT_Publish(&mqttClient, tBuf, pBuf, os_strlen(pBuf), 0, 0);
		    	os_sprintf(tBuf, "%s/fromesp/auto_humidity", sysCfg.base);
		    	os_sprintf(pBuf, "%d", humidity);
		    	MQTT_Publish(&mqttClient, tBuf, pBuf, os_strlen(pBuf), 0, 0);
		    	os_sprintf(tBuf, "%s/fromesp/auto_pressure", sysCfg.base);
		    	os_sprintf(pBuf, "%d", pressure);
		    	MQTT_Publish(&mqttClient, tBuf, pBuf, os_strlen(pBuf), 0, 0);
				}
			}

		else if (sysCfg.sensor_type==4)
			{
			BMP280_initialize(0x77);
			highTemperature=BMP280_getTemperature();
			pressure=roundDivision(BMP280_getPressure(),10);
			temperature=roundDivision(highTemperature,10);
			humidity=0;
			os_sprintf(tBuf, "%s/fromesp/auto_temperature", sysCfg.base);
			os_sprintf(pBuf, "%d", temperature);
			MQTT_Publish(&mqttClient, tBuf, pBuf, os_strlen(pBuf), 0, 0);
			os_sprintf(tBuf, "%s/fromesp/auto_pressure", sysCfg.base);
			os_sprintf(pBuf, "%d", pressure);
			MQTT_Publish(&mqttClient, tBuf, pBuf, os_strlen(pBuf), 0, 0);
			}
	}
}

LOCAL int IFA mod(int a, int b)
{
	int ret = a % b;
	if (ret < 0)
		ret += b;
	return ret;
}

LOCAL void IFA clock_cb(void *arg)
{
	if (sysCfg.clock != 255) // fancy clock on a port
	{
		wsBitMask = (uint8_t) sysCfg.clock;
		int hour12 = (tm.Hour % 12) * 5;

		int a;
		for (a = 0; a < 178; a += 3)
		{
			rgb.buffer[a] = 0;
			rgb.buffer[a + 1] = 0;
			rgb.buffer[a + 2] = 0;
		}
		rgb.buffer[mod(tm.Second * 3 + 2, 180)] = 255; // seconds hand blue
		rgb.buffer[mod(tm.Minute * 3 - 3, 180)] = 8; // minutes green
		rgb.buffer[mod(tm.Minute * 3, 180)] = 255; // minutes green
		rgb.buffer[mod(tm.Minute * 3 + 3, 180)] = 8; // minutes green
		int x = ((hour12) + (tm.Minute / 12)) * 3;
		rgb.buffer[mod(x - 8, 180)] = 5;
		rgb.buffer[mod(x - 5, 180)] = 16;
		rgb.buffer[mod(x - 2, 180)] = 80;
		rgb.buffer[mod(x + 1, 180)] = 255;
		rgb.buffer[mod(x + 4, 180)] = 80;
		rgb.buffer[mod(x + 7, 180)] = 16;
		rgb.buffer[mod(x + 10, 180)] = 5;
		WS2812OutBuffer(rgb.buffer, 180, 1); // 60 LEDs
	}
}

LOCAL void IFA rgb_cb(void *arg)
{
	int a, b, c, d, e, f, g;
	if (rgbPlayBuffer[rgbPlayPtr] == -1) rgbPlayPtr = 0;
	if ((rgbPlayBuffer[rgbPlayPtr] != -1) && (rgbDoSeq))
	{
		{
			b = rgbPlayBuffer[rgbPlayPtr++];
			c = rgbPlayBuffer[rgbPlayPtr++];
			d = rgbPlayBuffer[rgbPlayPtr++];
			e = rgbPlayBuffer[rgbPlayPtr++];
			f = rgbPlayBuffer[rgbPlayPtr++];
			g = rgbPlayBuffer[rgbPlayPtr++]; if (g < 5) g = 5;
			a = b * 3;
			while (a < ((b * 3) + (c * 3)))
			{
				rgb.buffer[a++] = e;
				rgb.buffer[a++] = d;
				rgb.buffer[a++] = f;
			}
		}
		WS2812OutBuffer(rgb.buffer, rgbTotal * 3, 1);
		os_timer_arm(&rgbTimer, g, 1);
	}
	else os_timer_arm(&rgbTimer, 250, 1); // tickover if not running
}

void IFA reboot(void)
{
	system_restart();
}

// handling power up - and lost signal - also handling the flashing light...

LOCAL void IFA lostThePlot_cb(void *arg)
{
	struct ip_info theIp;
	if (connectStatus & 16) // if not true - don't do anything - Aidan is handling setup
	{
		if (heartBeat == 0)
		{
			os_timer_disarm(&lostThePlotTimer);
			os_timer_disarm(&bounceTimer);
			os_timer_disarm(&startupTimer);
			os_timer_disarm(&temperatureTimer);
			os_timer_disarm(&rtcTimer);
			os_timer_disarm(&rgbTimer);
			os_timer_disarm(&ledTimer);
			os_timer_disarm(&clockTimer);
			reboot();
			return;
		}
		//iprintf(INFO,"WDT:%d\r\n",heartBeat);
		wifi_get_ip_info(0, &theIp);
		if (theIp.ip.addr)
		{
			if (tm.Valid) {
				setFlash(INDIC_TIME);

			}
			else {
				setFlash(INDIC_NO_TIME);
			}
			connectStatus |= 9;
			if ((connectStatus & 2) == 0)
			{
				iprintf(INFO, "Connected as %d:%d:%d:%d\r\n", theIp.ip.addr & 255, (theIp.ip.addr >> 8) & 255, (theIp.ip.addr >> 16) & 255, theIp.ip.addr >> 24);
				connectStatus |= 2;

				setFlashBack(INDIC_B_GOT_IP);

				mqtt_init();
			} // start up timers
			if ((connectStatus & 4) == 0) {
				iprintf(INFO, "MQTT connecting\r\n");
				MQTT_Disconnect(&mqttClient);
				mqtt_setup();
				MQTT_Connect(&mqttClient);
			}
		}
		else
		{
			if (connectStatus & 32)
			{
				connectStatus &= (255 - 32);
				os_timer_arm(&lostThePlotTimer, 4000, 1);
				iprintf(RESPONSE, "Speeding up Timer\r\n");
			}

			connectStatus &= (255 - 1);
			if (connectStatus & 4) {
				iprintf(INFO, "MQTT disconnected\r\n");
				connectStatus &= (255 - 4);
				MQTT_Disconnect(&mqttClient);
				setFlashBack(INDIC_B_LOSTMQTT);  e13OffSet = INDOFF;
			}
			if (connectStatus & 8)
			{
				if (heartBeat > 2) iprintf(RESPONSE, "Retrying AP: %s\r\n", (sysCfg.stationSwap) ? sysCfg.stationSsid2 : sysCfg.stationSsid);
				if (heartBeat > 20) heartBeat = 20;
			}
			else
			{
				if (heartBeat > 2) iprintf(RESPONSE, "Waiting for Access Point %s\r\n", (sysCfg.stationSwap) ? sysCfg.stationSsid2 : sysCfg.stationSsid);
				if (heartBeat > 20) heartBeat = 20;
			}
		}
		if (heartBeat == 2)
		{
			if (sysCfg.stationSwap == 0)
			{
				sysCfg.stationSwap = 1;
				iprintf(RESPONSE, "Switching to AP: %s\r\n", sysCfg.stationSsid2);
			}
			else
			{
				sysCfg.stationSwap = 0;
				iprintf(RESPONSE, "Switching to AP: %s\r\n", sysCfg.stationSsid);
			}
			cfgSave();
		}
		if (heartBeat) heartBeat--;
	}
}

LOCAL void IFA rtc_cb(void *arg)
{
	++myRtc;

	if (timeoutCount) // simple watchdog
	{
		--timeoutCount;
		if (timeoutCount == sysCfg.timeoutSecs) easygpio_outputSet(sysCfg.timeoutPin,  (sysCfg.timeoutInvert) ? OUT_OFF : OUT_ON);
		if (timeoutCount == 0) { easygpio_outputSet(sysCfg.timeoutPin,  (sysCfg.timeoutInvert) ? OUT_ON : OUT_OFF); if (sysCfg.timeoutRepeat) timeoutCount = sysCfg.timeoutPeriod; }
	}

	if (mist.repeats)
	{
		if (mist.activeCounter)
		{
			mist.activeCounter--;
			if (mist.activeCounter == 0) {
				mist.gapCounter = mist.gap;
				easygpio_outputSet(mist.device, OUT_OFF);
			}
		}
		else if (mist.gapCounter)
		{
			mist.gapCounter--;
			if (mist.gapCounter == 0) {
				mist.repeats--;
				if (mist.repeats) {
					mist.activeCounter = mist.active;
					easygpio_outputSet(mist.device, OUT_ON);
				}
			}
		}
	}

	if (tm.Valid) {
		timeTimeout = 0;
		tm.Valid--;
	}

	if (tm.Valid < 1000)     // no valid time? Time is sent on powerup (mqttConnect) and also every 24 hours so unit has dawn dusk info
	{
		if (timeTimeout++ == 120)     // 2 after minutes of no time ask for it every 2 mins. Should never be needed
		{
			char tBuf[TBUFSIZE];
			timeTimeout = 0;
			os_sprintf(tBuf, "{\"id\":\"%s\",\"desc\":\"%s\",\"attribute\":\"%s\",\"fail\":\"%d\"}",
			           sysCfg.base, sysCfg.description, sysCfg.attribute, sysCfg.lastFail); // changed to json
			MQTT_Publish(&mqttClient, "esplogon", tBuf, os_strlen(tBuf), 0, 0);
		}
	}

	convertTime();
	analogAccum = ((analogAccum * 9) / 10) + system_adc_read();
	analog = analogAccum / 10;

	if (sysCfg.out0Status == 6)
	{
		if (out0Timer)
		{
			easygpio_outputSet(sysCfg.relayoutput_override, (sysCfg.invert & 1) ? OUT_OFF : OUT_ON);
			out0Timer--;
		}
		else
		{
			easygpio_outputSet(sysCfg.relayoutput_override, (sysCfg.invert & 1) ? OUT_ON : OUT_OFF);
			sysCfg.out0Status = 0;
			cfgSave();
		}
	}

	if (sysCfg.out2Status == 6)
	{
		if (out2Timer)
		{
			easygpio_outputSet(2, (sysCfg.invert & 2) ? OUT_OFF : OUT_ON);
			out2Timer--;
		}
		else
		{
			easygpio_outputSet(2, (sysCfg.invert & 2) ? OUT_ON : OUT_OFF);
			sysCfg.out2Status = 0;
			cfgSave();
		}
	}

	if (sysCfg.out4Status == 6)
	{
		if (out4Timer)
		{
			easygpio_outputSet(4, (sysCfg.invert & 8) ? OUT_OFF : OUT_ON);
			out4Timer--;
		}
		else
		{
			easygpio_outputSet(4, (sysCfg.invert & 8) ? OUT_ON : OUT_OFF);
			sysCfg.out4Status = 0;
			cfgSave();
		}
	}

	if (sysCfg.out5Status == 6)
	{
		if (out5Timer)
		{
			easygpio_outputSet(5, (sysCfg.invert & 4) ? OUT_OFF : OUT_ON );
			out5Timer--;
		}
		else
		{
			easygpio_outputSet(5, (sysCfg.invert & 4) ? OUT_ON : OUT_OFF);
			sysCfg.out5Status = 0;
			cfgSave();
		}
	}

	if (sysCfg.out12Status == 6)
	{
		if (out12Timer)
		{
			easygpio_outputSet(12, (sysCfg.invert & 16) ? OUT_OFF : OUT_ON);
			out12Timer--;
		}
		else
		{
			easygpio_outputSet(12, (sysCfg.invert & 16) ? OUT_ON : OUT_OFF);
			sysCfg.out12Status = 0;
			cfgSave();
		}
	}

	if (sysCfg.out13Status == 6)
	{
		if (out13Timer)
		{
			easygpio_outputSet(13, (sysCfg.invert & 32) ? OUT_OFF : OUT_ON);
			out13Timer--;
		}
		else
		{
			easygpio_outputSet(13, (sysCfg.invert & 32) ? OUT_ON : OUT_OFF);
			sysCfg.out13Status = 0;
			cfgSave();
		}
	}


	// added as all the others have this function
	if (sysCfg.out14Status == 6) //nde (15-11-16) out14status
	{
	  if (out14Timer) {
	     easygpio_outputSet(14, (sysCfg.newInvert & 1) ? OUT_OFF : OUT_ON);
	     out14Timer--;
	   }
	  else {
	    easygpio_outputSet(14, (sysCfg.newInvert & 1) ? OUT_ON : OUT_OFF);
	    sysCfg.out14Status = 0;
	    cfgSave();
	  }
	}


	if (sysCfg.out15Status == 6)
	{
		if (out15Timer)
		{
			easygpio_outputSet(15, (sysCfg.invert & 64) ? OUT_OFF : OUT_ON);
			out15Timer--;
		}
		else
		{
			easygpio_outputSet(15, (sysCfg.invert & 64) ? OUT_ON : OUT_OFF);
			sysCfg.out15Status = 0;
			cfgSave();
		}
	}

	if (sysCfg.out16Status == 6)
	{
		if (out16Timer)
		{
			easygpio_outputSet(16, (sysCfg.invert & 128) ? OUT_OFF : OUT_ON);
			out16Timer--;
		}
		else
		{
			easygpio_outputSet(16, (sysCfg.invert & 128) ? OUT_ON : OUT_OFF);
			sysCfg.out16Status = 0;
			cfgSave();
		}
	}

	++secondsUp;

	if (tm.Second == 0)     // **** timed once every minute handles sysCfg.out_status
	{
		// need temporary minutes in the day
		int t_mins, doit;
		t_mins = tm.Hour;
		t_mins *= 60;
		t_mins += tm.Minute;

		switch (sysCfg.out0Status)
		{
		case 0:
			break;     // covered in the OUT0 command
		case 1:
			if (tm.Valid == 0)
				easygpio_outputSet(sysCfg.relayoutput_override, (sysCfg.invert));
			break;     // covered in the OUT0 command - but if no time available, turn OUT0 off after a day
		case 2:
			if (t_mins > sysCfg.dusk) // 2 means on after dusk
				easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.invert & 1) ^ 1));
			else
				easygpio_outputSet(sysCfg.relayoutput_override, (sysCfg.invert & 1));
			break;
		case 3:
			if ((t_mins > sysCfg.dusk) || (t_mins < sysCfg.dawn)) // 3 means on all night
				easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.invert & 1) ^ 1));
			else
				easygpio_outputSet(sysCfg.relayoutput_override, (sysCfg.invert & 1));
			break;
		case 4:
			if ((t_mins > sysCfg.dawn) && (t_mins < sysCfg.dusk)) // 4 means on all day
				easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.invert & 1) ^ 1));
			else
				easygpio_outputSet(sysCfg.relayoutput_override, (sysCfg.invert & 1));
			break;
		case 5: // heating control
			doit = 0;
			if (sysCfg.onOne > sysCfg.offOne) {
				if ((t_mins > sysCfg.onOne) || (t_mins < sysCfg.offOne)) doit = 1;
			}
			else {
				if ((t_mins > sysCfg.onOne) && (t_mins < sysCfg.offOne)) doit = 1;
			}
			if (sysCfg.onOne > sysCfg.offTwo) {
				if ((t_mins > sysCfg.onTwo) || (t_mins < sysCfg.offTwo)) doit = 1;
			}
			else {
				if ((t_mins > sysCfg.onTwo) && (t_mins < sysCfg.offTwo)) doit = 1;
			}

			if (doit) {
				if (temperature < sysCfg.peak)
					easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.invert & 1) ^ 1));
				else
					easygpio_outputSet(sysCfg.relayoutput_override, (sysCfg.invert & 1));
			}
			else     // if not peak - OR if the time is not set due to bad connection, say  -default to off-peak
			{
				if (temperature < sysCfg.offPeak)
					easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.invert & 1) ^ 1));
				else
					easygpio_outputSet(sysCfg.relayoutput_override, (sysCfg.invert & 1));
			}
			break;

		case 6: // handled in seconds area - downcounters
			break;

		default: // from here - heating timeout.... ie from 7 onwards is number of days for standby

			if (sysCfg.out0Status<1000) // not sure if anyone actually uses this but I'll reserve 1000+ for new commands
			{
			if ((tm.Minute == 0) && (tm.Hour == 0)) {
				sysCfg.out0Status--; // drop down once a day....
				cfgSave();
			}
			// frost setting if over 5
			if (sysCfg.out0Status > 6) {
				if (temperature < sysCfg.frost)
					easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.invert & 1) ^ 1));
				else
					easygpio_outputSet(sysCfg.relayoutput_override, (sysCfg.invert & 1));
			}
			}
			break;
		}
	}
}

//
// This function parses the incoming data, generally from the MQTT - but COULD be from serial
// and processes commands.
//

#define WASSERIAL 2
void IFA mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char topicBuf[topic_len + 1];
	char dataBuf[data_len + 1];
	char tBuf[84];
	uint8_t messageType;

	struct ip_info theIp;

	heartBeat = TICKS; // reset comms timer - as clearly we have something!!!
	MQTT_Client* client = (MQTT_Client*) args;
	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;
	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;
	messageType = 0;     // my input parser handles multiple commands with up to 6 arguments - the first argument can be a string.
	os_sprintf(tBuf, "%s/toesp", sysCfg.base);
	if (dataBuf[0] == '{') {
		messageType = 1;     // if it has an opening squiggle - type 1
		if (os_strcmp(topicBuf, "serial$") == 0) messageType = WASSERIAL;     // if it is for me particularly it is a type 2
	}

	if (messageType) {
		char *bufPtr;
		char *tokenPtr;
		char strValue[84];     // could be passing a sizable message
		char strValue2[84];
		char token[84];        // don't need it this big but reused in EXT
		int32_t intValue;
		int32_t arg1;
		int32_t arg2;
		int32_t arg3;
		int32_t arg4;
		int32_t arg5;
		int32_t arg6;
		int32_t arg7;
		int32_t arg8;

		int16_t argCount;
		int8_t argType;
		int8_t negative;
		uint8_t doUpdate;
		uint8_t isQuery;
		uint8_t noArgs;
		uint8_t gotNum;
		uint8_t string2;

		bufPtr = dataBuf + 1;
		doUpdate = 0;
		while ((*bufPtr) && (*bufPtr != '}')) {     // Sequence through multiple action/value sets and their associated actions
			strValue[0] = 0;
			token[0] = 0;
			tokenPtr = token;
			intValue = -1;
			arg1 = -1;
			arg2 = -1;
			arg3 = -1;
			arg4 = -1;
			arg5 = -1;
			arg6 = -1;
			arg7 = -1;
			arg8 = -1;

			argCount = 0;
			isQuery = 0;
			noArgs = 0;
			string2 = 0;
			while (*bufPtr == ' ')
				bufPtr++;
			while ((*bufPtr) && (*bufPtr != ':') && (*bufPtr != ';') && (*bufPtr != '=') && (*bufPtr != '}')) {     // firstly get a token

				if ((*bufPtr == '\'') || (*bufPtr == '"')) bufPtr++;
				else {
					*tokenPtr++ = *bufPtr++;
					*tokenPtr = 0;     // build up command
				}
			}     // end of get a token
			if (*(tokenPtr - 1) == '?') {
				*(tokenPtr - 1) = 0;
				isQuery = 1;
			}
			else {     // only arguments here if it was NOT a query
				while (*bufPtr == ' ')
					bufPtr++;     // kill whitespace(s)
				if ((*bufPtr == ':') || (*bufPtr == '='))     // should be colon but accept =
				{
					bufPtr++;
					while ((*bufPtr) && (*bufPtr != ';') && (*bufPtr != '}'))     // look for arguments while no semicolon, no end of file and no closing brace
					{
						// now to build up arguments - only one string argument allowed... that goes into stringVar

						if (*bufPtr == '"')     // ***** build up a String?
						{
							bufPtr++;
							if (string2 == 1) tokenPtr = strValue2; else tokenPtr = strValue;  // re-use pointer
							while ((*bufPtr) && (*bufPtr != '"')) {
								if (*bufPtr == '\\')
								{
									uint8_t tmp1;
									switch (*++bufPtr)
									{
									case 'r' : *bufPtr = 0x0d; break;
									case 'e' : *bufPtr = 0x1b; break; // esc
									case 'n' : *bufPtr = 0x0a; break;
									case 't' : *bufPtr = 0x09; break;
									case 'x' : *(bufPtr + 2) = ((hexdec(*(bufPtr + 1)) << 4) + hexdec(*(bufPtr + 2)));
										++bufPtr; ++bufPtr; break;
									}
								}
								*tokenPtr++ = *bufPtr++;
								*tokenPtr = 0;
							}
							string2 = 1;
							bufPtr++;
							argCount++;
							continue;
						}
						// wasn't a string if we got this far, it's a numerical argument
						if (*bufPtr == '-') {     // check for negative numbers
							negative = 1;
							bufPtr++;
						}
						else negative = 0;
						intValue = 0;
						uint8_t hexType;

						if ((*bufPtr == '0') && (*(bufPtr + 1) == 'x')) {
							bufPtr += 2;
							hexType = 1;
						}
						else hexType = 0;     // Here we see if this is a hex number
						// now to collect the decimal or hex number
						gotNum = 0;
						if (hexType == 0) while ((*bufPtr >= '0') && (*bufPtr <= '9')) {
								gotNum = 1;
								intValue *= 10;
								intValue += (*bufPtr++ - '0');
							}
						else while (((*bufPtr >= '0') && (*bufPtr <= '9')) || ((*bufPtr >= 'a') && (*bufPtr <= 'f'))) {
								gotNum = 1;
								intValue *= 16;
								if ((*bufPtr >= 'a') && (*bufPtr <= 'f')) intValue += (*bufPtr++ - 'a' + 10);
								else intValue += (*bufPtr++ - '0');
							}
						if (gotNum == 0) {
							bufPtr++;
							continue;
						}
						if (negative) intValue = -intValue;     // at this point we have a number in intValue
						argCount++;
						switch (argCount)
						{
						case 1:
							arg1 = intValue;
							break;
						case 2:
							arg2 = intValue;
							break;
						case 3:
							arg3 = intValue;
							break;
						case 4:
							arg4 = intValue;
							break;
						case 5:
							arg5 = intValue;
							break;
						case 6:
							arg6 = intValue;
							break;
						case 7:
							arg7 = intValue;
							break;
						case 8:
							arg8 = intValue;
							break;
						}
					}     // end of looping through arguments
				}
				else bufPtr++;
			}     // end of checking for arguments - now to process this command

			if ((*bufPtr == ';') || (*bufPtr == '}') || (*bufPtr == ' ')) bufPtr++;

			// firstly set up the return token.... for return ESP messages
			os_sprintf(tBuf, "%s/fromesp/%s", sysCfg.base, token);

			if (os_strcmp(token, "ext") == 0) {
				char *gotat = strchr(strValue, '~');
				if (gotat != NULL) {
					*gotat = 0;     // separate the strings
					os_strcpy(token, strValue);
					os_sprintf(&token[gotat - strValue], "%lu", myRtc);
					gotat++;
					os_strcat(token, gotat);
					os_strcpy(strValue, token);
				}
				iprintf(RESPONSE, "{%s}\r\n", strValue);
			}

			else if (os_strcmp(token, "scan") == 0) {
				iprintf(RESPONSE, "Scanning..\r\n");
				wifi_station_scan(NULL, wifi_station_scan_done);
			}

			else if (os_strcmp(token, "nextion") == 0) {
				char *gotat = strchr(strValue, '~');
				if (gotat != NULL) {
					*gotat = 0;     // separate the strings
					os_strcpy(token, strValue);
					os_sprintf(&token[gotat - strValue], "%lu", myRtc);
					gotat++;
					os_strcat(token, gotat);
					os_strcpy(strValue, token);
				}
				iprintf(RESPONSE, "\xff\xff\xff%s\xff\xff\xff", strValue);
			}

			else if (os_strcmp(token, "port2out") == 0) { // 1=output
				if (isQuery) {
					os_sprintf(strValue, "%d", sysCfg.set2Out);
				}
				else {
					if (sysCfg.set2Out != intValue) { sysCfg.set2Out = intValue; doUpdate = 1; }
					if (sysCfg.set2Out == 1)
					{
						easygpio_pinMode(2, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
						(sysCfg.invert & 2) ? easygpio_outputSet(2, ((sysCfg.out2Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(2, ((sysCfg.out2Status == 1) ? OUT_ON : OUT_OFF));
					}
					else
						easygpio_pinMode(2, EASYGPIO_PULLUP, EASYGPIO_INPUT);
					ok();
				}
			}

			else if (os_strcmp(token, "temp_type") == 0) {  // 0=dallas, 1=DHT22 2=DHT11  3=BME280  4=BMP280
				if (isQuery) {
					os_sprintf(strValue, "%d", sysCfg.sensor_type);
				}
				else
				{
					if ((sysCfg.sensor_type != intValue) && (intValue < 255)) { gotDsReading=0; sysCfg.sensor_type = intValue; doUpdate = 1; }
					ok();
				}
			}

			else if (os_strcmp(token, "user") == 0) {
				if (isQuery) {
					os_sprintf(strValue, "%d", sizeof(sysCfg));
				}

			}

			else if (os_strcmp(token, "time") == 0) {
				if ((!isQuery) && (intValue != 0))
				{
					tm.Valid = 86400;
					myRtc = intValue;
					convertTime();
					setFlash(INDIC_TIME);  if (sysCfg.rgb_ind) e13OffSet = INDOFF; else e13OffSet = 80;
				}
				os_sprintf(strValue, "Time is %02d:%02d:%02d %02d/%02d/%02d\r\n", tm.Hour, tm.Minute, tm.Second, tm.Day, tm.Month, tm.Year);
			}

			else if (os_strcmp(token, "dawn") == 0) {
				if ((sysCfg.dawn != intValue) && (intValue != 0)) {
					sysCfg.dawn = intValue;
					doUpdate = 1;
				}
				ok();
			}

			else if (os_strcmp(token, "dusk") == 0) {
				if ((sysCfg.dusk != intValue) && (intValue != 0)) {
					sysCfg.dusk = intValue;
					doUpdate = 1;
				}
				ok();
			}

			else if (os_strcmp(token, "peak") == 0) {
				if (sysCfg.peak != intValue) {
					sysCfg.peak = intValue;
					doUpdate = 1;
				}
				ok();
			}
			else if (os_strcmp(token, "off_peak") == 0) {
				if (sysCfg.offPeak != intValue) {
					sysCfg.offPeak = intValue;
					doUpdate = 1;
				}
				ok();
			}
			else if (os_strcmp(token, "frost") == 0) {
				if (sysCfg.frost != intValue) {
					sysCfg.frost = intValue;
					doUpdate = 1;
				}
				ok();
			}
			else if (os_strcmp(token, "on_1") == 0) {
				if (sysCfg.onOne != intValue) {
					sysCfg.onOne = intValue;
					doUpdate = 1;
				}
				ok();
			}
			else if (os_strcmp(token, "off_1") == 0) {
				if (sysCfg.offOne != intValue) {
					sysCfg.offOne = intValue;
					doUpdate = 1;
				}
				ok();
			}

			else if (os_strcmp(token, "on_2") == 0) {
				if (sysCfg.onTwo != intValue) {
					sysCfg.onTwo = intValue;
					doUpdate = 1;
				}
				ok();
			}

			else if (os_strcmp(token, "off_2") == 0) {
				if (sysCfg.offTwo != intValue) {
					sysCfg.offTwo = intValue;
					doUpdate = 1;
				}
				ok();
			}
			else if (os_strcmp(token, "enable13") == 0) {
				if (sysCfg.enable13 != intValue) {
					sysCfg.enable13 = intValue;
					doUpdate = 1;
				}
				ok();
			}

			else if (os_strcmp(token, "sonoff") == 0) { // 1 or 0 - merely inverts on-off ratio of flashing light indicator
				sysCfg.sonoff = intValue & 1;
				doUpdate = 1; ok();
			}


			else if (os_strcmp(token, "wifi_button") == 0) { // which pin is used for WIFI setup must reboot after this - 0 or 2
				sysCfg.wifi_button = intValue & 2;
				if (sysCfg.wifi_button == 2) sysCfg.relayoutput_override = 0; else sysCfg.relayoutput_override = 16;
				doUpdate = 1; ok();
			}

			else if (os_strcmp(token, "verbose") == 0) {
				enable_debug_messages = INFO | RESPONSE;
			}
			else if (os_strcmp(token, "no_serial") == 0) {
				enable_debug_messages = 0;
			}

			else if (os_strcmp(token, "reboot_now") == 0) {
				reboot();
			}

			else if (os_strcmp(token, "heartbeat") == 0) {
				iprintf(RESPONSE, "Tick\r\n");
			}

			else if (os_strcmp(token, "calibrate") == 0) {
				if (isQuery) {
					os_sprintf(strValue, "%d", sysCfg.calibrate);
				}
				else {
					if (sysCfg.calibrate != intValue) {
						sysCfg.calibrate = intValue;
						doUpdate = 1;
					}
					ok();
				}
			}


			else if (os_strcmp(token,"si1132_visible") ==0) {
			    if (si_setup==0) { ODROID_Si1132_begin(); si_setup=1; }
				isQuery=1;
				os_sprintf(strValue, "%d", ODROID_Si1132_readVisible());
			}

			else if (os_strcmp(token,"si1132_uv") ==0) {
			    if (si_setup==0) { ODROID_Si1132_begin(); si_setup=1; }
				isQuery=1;
				os_sprintf(strValue, "%d", ODROID_Si1132_readUV());
			}
			else if (os_strcmp(token,"si1132_ir") ==0) {
				if (si_setup==0) { ODROID_Si1132_begin(); si_setup=1; }
				isQuery=1;
				os_sprintf(strValue, "%d", ODROID_Si1132_readIR());
			}


			else if (os_strcmp(token, "temperature_port") == 0) {
				if (isQuery) {
					os_sprintf(strValue, "%d", sysCfg.temperaturePort);
				}
				else {
					if ((intValue == 2) || (intValue == 14) || (intValue == 0))
					{
						if (sysCfg.temperaturePort != intValue) {
							sysCfg.temperaturePort = intValue;
							gotDsReading&=(0xffff-(1<<intValue));
							if ((intValue == 2) || (intValue == 14) ) easygpio_pinMode(sysCfg.temperaturePort, EASYGPIO_PULLUP, EASYGPIO_INPUT);
							doUpdate = 1;
						}
					}
					ok();
				}
			}

			// syscfg.invert
			// bit 0 = out0
			// bit 1 = out2
			// bit 2 = out4
			// bit 3 = out5
			// bit 4 = out12
			// bit 5 = out13
			// bit 6 = out15

			else if (os_strcmp(token, "invert") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", sysCfg.invert||(sysCfg.newInvert<<8));
				else {
					if (sysCfg.invert != (intValue&255)) {
						sysCfg.invert = (intValue&255);
						doUpdate = 1;
					}
					if (sysCfg.newInvert != (intValue>>8)) { // ran out of inverts - needed one for GPIO14
						sysCfg.newInvert = (intValue>>8);
						doUpdate = 1;
					}

					ok();
				}
			}

			else if (os_strcmp(token, "invertset") == 0) {
				sysCfg.invert |= (intValue&255);
				sysCfg.newInvert |= (intValue>>8);
				doUpdate = 1; ok();
			}

			else if (os_strcmp(token, "polling_interval") == 0) {
				if (arg1>=5)
				{
					os_timer_disarm(&temperatureTimer);
					os_timer_setfn(&temperatureTimer, (os_timer_func_t *) temperature_cb, (void *) 0);
					os_timer_arm(&temperatureTimer,arg1*1000, 1);
					ok();
				}
			}

			else if (os_strcmp(token, "enable16") == 0) {
			if (isQuery) os_sprintf(strValue, "%d",  sysCfg.enable16);
			else
				{
				sysCfg.enable16 = intValue; doUpdate = 1; ok();
				}
			}

			else if (os_strcmp(token, "invertreset") == 0) {
				sysCfg.invert &= (255 - (intValue&255));
				sysCfg.newInvert &= (255 - (intValue>>8));
				doUpdate = 1; ok();
			}

			else if (os_strcmp(token, "mqtt") == 0) {
				MQTT_Publish(&mqttClient, strValue, strValue2, os_strlen(strValue2), 0, 0);
				ok();
			}

			else if (os_strcmp(token, "set_serial") == 0) {
				if (isQuery) os_sprintf(strValue, "%d",  sysCfg.serial2);
				else
				{
					sysCfg.serial2 = intValue;
					cfgSave();
					system_restart();
				}
			}

			else if (os_strcmp(token, "to_serial2") == 0) {
				if (os_strlen(strValue) >= 1)
				{
					//os_sprintf(tBuf, "%s", strValue);
					Softuart_Puts(&softuart2, strValue);
				}
				ok();
			}

			else if (os_strcmp(token, "to_nextion") == 0) {
				if (os_strlen(strValue) >= 1)
				{
					// os_sprintf(tBuf, "%s", strValue);
					Softuart_Writeline_Nextion(&softuart2, strValue);
				}
				ok();
			}

			else if (os_strcmp(token, "debug") == 0) {
				iprintf(RESPONSE, "Time: %02d:%02d:%02d %02d/%02d/%02d\r\nTime Code: %lu\r\nDusk: %02d:%02d Dawn: %02d:%02d\r\n", tm.Hour, tm.Minute, tm.Second,
				        tm.Day, tm.Month, tm.Year, myRtc, sysCfg.dusk / 60, sysCfg.dusk % 60, sysCfg.dawn / 60, sysCfg.dawn % 60);
				iprintf(RESPONSE, "On1: %02d:%02d Off1: %02d:%02d On2: %02d:%02d Off2: %02d:%02d Peak: %dc Off-peak: %dc Frost: %dc \r\n", sysCfg.onOne / 60,
				        sysCfg.onOne % 60, sysCfg.offOne / 60, sysCfg.offOne % 60, sysCfg.onTwo / 60, sysCfg.onTwo % 60, sysCfg.offTwo / 60, sysCfg.offTwo % 60,
				        sysCfg.peak, sysCfg.offPeak, sysCfg.frost);

				wifi_get_ip_info(0, &theIp);
				iprintf(RESPONSE, "IP: %d:%d:%d:%d\r\n", theIp.ip.addr & 255, (theIp.ip.addr >> 8) & 255, (theIp.ip.addr >> 16) & 255, theIp.ip.addr >> 24);
				iprintf(RESPONSE, "Internal ID: %s \r\n", sysCfg.deviceId);
				iprintf(RESPONSE, "ID: %s \r\n", sysCfg.base);
				iprintf(RESPONSE, "DESC: %s \r\n", sysCfg.description);
				iprintf(RESPONSE, "FLAGS: %s \r\n", sysCfg.attribute);
				iprintf(RESPONSE, (sysCfg.stationSwap == 0) ? "SSID: %s (Active)" : "SSID: %s" , sysCfg.stationSsid);
				iprintf(RESPONSE, "  Pass: %s \r\n", sysCfg.stationPwd);
				iprintf(RESPONSE, (sysCfg.stationSwap == 1) ? "SSID2: %s (Active)" : "SSID2: %s" , sysCfg.stationSsid2);
				iprintf(RESPONSE, "  Pass2: %s \r\n", sysCfg.stationPwd2);
				iprintf(RESPONSE, "MQTT Host: %s", sysCfg.mqttHost);
				iprintf(RESPONSE, " Port: %d", sysCfg.mqttPort);
				iprintf(RESPONSE, " User: %s \r\n", sysCfg.mqttUser);
				iprintf(RESPONSE, "OTA Host: %s", sysCfg.otaHost);
				iprintf(RESPONSE, "  Port: %d \r\n", sysCfg.otaPort);
				iprintf(RESPONSE, "Code Version: %s \r\n", SYSTEM_VER);
				iprintf(RESPONSE, "SDK Version: %s \r\n", system_get_sdk_version());
				iprintf(RESPONSE, "RSSI: %d \r\n",  wifi_station_get_rssi());

				iprintf(RESPONSE, "Out0: %d (%d with invert)\r\n",  sysCfg.out0Status, ((sysCfg.out0Status & 1) ^ ((sysCfg.invert >> 0) & 1)));
				iprintf(RESPONSE, "Out4: %d (%d with invert)\r\n",  sysCfg.out5Status, ((sysCfg.out5Status & 1) ^ ((sysCfg.invert >> 2) & 1))); //deliberately swapped with 5
				iprintf(RESPONSE, "Out5: %d (%d with invert)\r\n",  sysCfg.out4Status, ((sysCfg.out4Status & 1) ^ ((sysCfg.invert >> 3) & 1)));
				iprintf(RESPONSE, "Out12: %d (%d with invert)\r\n",  sysCfg.out12Status, ((sysCfg.out12Status & 1) ^ ((sysCfg.invert >> 4) & 1)));
				iprintf(RESPONSE, "Out15: %d (%d with invert)\r\n",  sysCfg.out15Status, ((sysCfg.out15Status & 1) ^ ((sysCfg.invert >> 5) & 1)));
				if (sysCfg.enable16) iprintf(RESPONSE, "Out16: %d (%d with invert)\r\n",  sysCfg.out16Status, ((sysCfg.out16Status & 1) ^ ((sysCfg.invert >> 6) & 1)));
				iprintf(RESPONSE, "Sensor Type: ");
				if (sysCfg.sensor_type == 0) iprintf(RESPONSE, "Dallas 1880\r\n");
				else if (sysCfg.sensor_type == 1) iprintf(RESPONSE, "DHT22\r\n");
				else if (sysCfg.sensor_type == 2) iprintf(RESPONSE, "DHT11\r\n");
				else if (sysCfg.sensor_type == 3) iprintf(RESPONSE, "BME280\r\n");
				else if (sysCfg.sensor_type == 4) iprintf(RESPONSE, "BMP280\r\n");
				else iprintf(RESPONSE, "Nothing\r\n");

				iprintf(RESPONSE, "Temperature Port: ");
				if (sysCfg.temperaturePort == 2) iprintf(RESPONSE, "GPIO2\r\n");
				else if (sysCfg.temperaturePort == 14) iprintf(RESPONSE, "GPIO14\r\n");
				else  iprintf(RESPONSE, "None\r\n");
				iprintf(RESPONSE, "GPIO13 available for general use: "); iprintf(RESPONSE, sysCfg.enable13 ? "Yes\r\n" : "No\r\n");
				iprintf(RESPONSE, "GPIO2 is an "); iprintf(RESPONSE, (sysCfg.set2Out) ? "output\r\n" : "input\r\n");
				iprintf(RESPONSE, "Sonoff setting=%d\r\n", sysCfg.sonoff);
				iprintf(RESPONSE, "WiFi button=%d\r\n", sysCfg.wifi_button);
				iprintf(RESPONSE, "Invert settings=HEX(%2x)\r\n", sysCfg.invert);
				iprintf(RESPONSE, "Serial2 settings=%d\r\n", sysCfg.serial2);
				iprintf(RESPONSE, "RGB Indicator=%d\r\n", sysCfg.rgb_ind);
				iprintf(RESPONSE, "Electrodragon=%d\r\n", sysCfg.electrodragon);
				if (sysCfg.clock == 255) iprintf(RESPONSE, "No LED clock \r\n"); else iprintf(RESPONSE, "LED Clock on port %d\r\n", sysCfg.clock);
				iprintf(RESPONSE, "CPU frequency: %dMhz \r\n",  system_get_cpu_freq());
				iprintf(RESPONSE, "Free Heap: %d bytes\r\n",  system_get_free_heap_size());
				iprintf(RESPONSE, "Up Time: %d:%02d:%02d \r\n",  secondsUp / (60 * 60), (secondsUp / 60) % 60, secondsUp % 60);
			}
			else if (os_strcmp(token, "temperature") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", temperature);
				else temperature = intValue;
			}

			else if (os_strcmp(token, "hightemperature") == 0) {
				os_sprintf(strValue, "%d", highTemperature);
			}

			else if (os_strcmp(token, "humidity") == 0) {
				os_sprintf(strValue, "%d", humidity);
			}

			else if (os_strcmp(token, "pressure") == 0) {
				os_sprintf(strValue, "%d", pressure);
			}

			else if (os_strcmp(token, "uptime") == 0) {
				os_sprintf(strValue, "%d:%02d:%02d", secondsUp / (60 * 60), (secondsUp / 60) % 60, secondsUp % 60);
			}

			else if (os_strcmp(token, "adc") == 0) {
				os_sprintf(strValue, "%d", analog);
			}

			else if (os_strcmp(token, "flash_erase") == 0) {
				spi_flash_erase_sector(arg1+0x200);
				ok();
			}

			else if (os_strcmp(token, "rgb_lcd_setup") == 0) {
				rgb_lcd_begin(16,2,LCD_5x8DOTS);
				rgb_lcd_display();
				ok();
			}

			else if (os_strcmp(token, "rgb_lcd_write") == 0) {
			    char *rgb_s;
			    rgb_s=strValue;
			    while (*rgb_s) rgb_lcd_write(*rgb_s++);
				ok();
			}
			else if (os_strcmp(token, "rgb_lcd_background") == 0) {
			    rgb_lcd_setRGB(arg1,arg2,arg3);
				ok();
			}
			else if (os_strcmp(token, "rgb_lcd_clear") == 0) {
			    rgb_lcd_clear();
				ok();
			}
			else if (os_strcmp(token, "rgb_lcd_cursor") == 0) {
			    rgb_lcd_setCursor(arg1,arg2);
				ok();
			}


			else if (os_strcmp(token, "flash_write") == 0) {
				spi_flash_write(arg1+0x200000,&arg2,4);
				ok();
			}

			else if (os_strcmp(token, "flash_read") == 0) {
				spi_flash_read(arg1+0x200000,&arg2,4);
				os_sprintf(strValue, "%d", arg2);
				isQuery=1;
			}

			else if (os_strcmp(token, "fla1") == 0) {
				my_flash_write(0,0,&arg1,4); // sector (0-253 available), offset into block, buffer and buffer size (must be 4092 or less chars)
				ok();
			}

			else if (os_strcmp(token, "fla2") == 0) {
				uint32_t fred;
				my_flash_read(0,0,&fred,4);
				os_sprintf(strValue, "%d", fred);
				isQuery=1;
			}

			else if (os_strcmp(token, "ver") == 0) {
				os_sprintf(strValue, "%s", SYSTEM_VER);
			}

			else if (os_strcmp(token, "heap")==0){
			os_sprintf(strValue, "%d", system_get_free_heap_size());
			}

			else if (os_strcmp(token, "voltage") == 0) {
				os_sprintf(strValue, "%d.%02d", (analog * 1000 / sysCfg.calibrate) / 100, (analog * 1000 / sysCfg.calibrate) % 100);
			}

			else if (os_strcmp(token, "id") == 0) {
				if (isQuery) os_sprintf(strValue, "%s",  sysCfg.base);
				else {
					if (os_strlen(strValue) >= 2) {
						os_strcpy(sysCfg.base, strValue);
						doUpdate = 1; ok();
					}
					else iprintf(RESPONSE, "Bad ID\r\n");
				}
			}

			else if (os_strcmp(token, "mist") == 0) {
				mist.device = arg1;
				mist.active = arg2;
				mist.activeCounter = arg2;
				mist.gap = arg3;
				mist.gapCounter = arg3;
				mist.repeats = arg4;
				easygpio_outputSet(arg1, OUT_ON);
				ok();
			}

			else if (os_strcmp(token, "led_timer") == 0) {
				if (arg1 < 20) arg1 = 20;
				os_timer_disarm(&ledTimer);
				os_timer_arm(&ledTimer, arg1, 1);
				ok();
			}

			else if (os_strcmp(token, "desc") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", sysCfg.description);
				else {
					os_strcpy(sysCfg.description, strValue);
					doUpdate = 1;
					ok();
				}
			}

			else if (os_strcmp(token, "warning") == 0) { // led indicator warning r,g,b,duration (*25ms)
				setFlashBack(arg1, arg2, arg3, arg4); e13OffSet = INDOFF;
				ok();

			}

			else if (os_strcmp(token, "rgb_ind") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", sysCfg.rgb_ind);
				else  {
					sysCfg.rgb_ind = intValue;
					doUpdate = 1; ok();
				}
			}

			else if (os_strcmp(token, "attrib") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", sysCfg.attribute);
				else
				{
					os_strcpy(sysCfg.attribute, strValue);
					doUpdate = 1;
					ok();
				}
			}

			else if (os_strcmp(token, "fixed_ip") == 0) {
				struct ip_info info;
				wifi_station_dhcpc_stop();
				wifi_softap_dhcps_stop();
				IP4_ADDR(&info.ip, arg2, arg3, arg4, arg5);
				IP4_ADDR(&info.gw, arg2, arg3, arg4, arg6);
				IP4_ADDR(&info.netmask, 255, 255, 255, 0);
				wifi_set_ip_info(arg1, &info);
				ok();
			}

			else if (os_strcmp(token, "rssi") == 0) {
				int16_t rssi;
				rssi = wifi_station_get_rssi();
				os_sprintf(strValue, "%d", rssi);
			}

			else if (os_strcmp(token, "ip_info") == 0) {
				// this stuff lifted clean from Jeroem Domberg's code here
				// https://github.com/jeelabs/esp-link/blob/master/esp-link/cgiwifi.c#L558
				struct ip_info info;
				if (wifi_get_ip_info(0, &info)) {
					iprintf(RESPONSE, "ip: %d.%d.%d.%d\r\n", IP2STR(&info.ip.addr));
					iprintf(RESPONSE, "netmask: %d.%d.%d.%d\r\n", IP2STR(&info.netmask.addr));
					iprintf(RESPONSE, "gateway: %d.%d.%d.%d\r\n", IP2STR(&info.gw.addr));
				} else {
					iprintf(RESPONSE, "ip: -none-\r\n");
				}

				struct station_config stconf;
				wifi_station_get_config(&stconf);

				static char *connStatuses[] = { "idle", "connecting", "wrong password", "AP not found",
				                                "failed", "got IP address"
				                              };

				static char *wifiPhy[]  = { 0, "11b", "11g", "11n" };
				static char *wifiMode[] = { 0, "STA", "AP", "AP+STA" };
				uint8_t op = wifi_get_opmode() & 0x3;
				char *mode = wifiMode[op];
				char *status = "unknown";
				int st = wifi_station_get_connect_status();

				if (st >= 0 && st < sizeof(connStatuses)) status = connStatuses[st];
				int p = wifi_get_phy_mode();
				char *phy = wifiPhy[p & 3];

				sint8 rssi = wifi_station_get_rssi();
				if (rssi > 0) rssi = 0;
				uint8 mac_addr[6];
				wifi_get_macaddr(0, mac_addr);
				uint8_t chan = wifi_get_channel();

				iprintf(RESPONSE,
				        "mode: %s\r\nssid: %s\r\nstatus: %s\r\nphy: %s\r\n"
				        "rssi: %ddB\r\nmac:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
				        mode, (char*)stconf.ssid, status, phy, rssi,
				        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
				iprintf(RESPONSE, "channel:%d\r\n", (uint16_t) chan);
			}

			else if (os_strcmp(token, "ssid") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", sysCfg.stationSsid);
				else
				{
					if (os_strlen(strValue) > 1)
					{
						os_strcpy(sysCfg.stationSsid, strValue);
						doUpdate = 1;
					}
					ok();
				}
			}

			else if (os_strcmp(token, "pass") == 0) {
				os_strcpy(sysCfg.stationPwd, strValue);
				doUpdate = 1;
				ok();
			}

			else if (os_strcmp(token, "ssid2") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", sysCfg.stationSsid2);
				else
				{
					if (os_strlen(strValue) > 1)
					{
						os_strcpy(sysCfg.stationSsid2, strValue);
						doUpdate = 1;
					}
					ok();
				}
			}

			else if (os_strcmp(token, "swap") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", sysCfg.stationSwap);
				else
				{
					sysCfg.stationSwap = intValue & 255;
					doUpdate = 1;
					ok();
				}
			}

			else if (os_strcmp(token, "web_setup") == 0) {
				sysCfg.wifiSetup = 1;
				cfgSave();
				system_restart();
			}

			else if (os_strcmp(token, "pass2") == 0) {
				os_strcpy(sysCfg.stationPwd2, strValue);
				doUpdate = 1;
				ok();
			}

			else if (os_strcmp(token, "ssid_retries") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", sysCfg.ssid_retries);
				else
				{
					sysCfg.ssid_retries = intValue;
					doUpdate = 1;
					ok();
				}
			}

			else if (os_strcmp(token, "mqtt_host") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", sysCfg.mqttHost);
				else
				{
					if (os_strlen(strValue) > 1)
					{
						os_strcpy(sysCfg.mqttHost, strValue);
						doUpdate = 1;
					}
					ok();
				}
			}

			else if (os_strcmp(token, "mqtt_port") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", sysCfg.mqttPort);
				else
				{
					sysCfg.mqttPort = intValue;
					//strcpy(sysCfg.mqttPort, strValue); WRONG!!
					doUpdate = 1; ok();
				}
			}

			else if (os_strcmp(token, "ota_host") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", sysCfg.otaHost);
				else
				{
					os_strcpy(sysCfg.otaHost, strValue);
					doUpdate = 1; ok();
				}
			}
			else if (os_strcmp(token, "ota_port") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", sysCfg.otaPort);
				else
				{
					sysCfg.otaPort=intValue;
					doUpdate = 1; ok();
				}
			}

			else if (os_strcmp(token, "mqtt_user") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", sysCfg.mqttUser);
				else
				{
					os_strcpy(sysCfg.mqttUser, strValue);
					doUpdate = 1; ok();
				}
			}

			else if (os_strcmp(token, "mqtt_pass") == 0) {
				if (isQuery) os_sprintf(strValue, "*******");
				else
				{
					os_strcpy(sysCfg.mqttPass, strValue);
					doUpdate = 1; ok();
				}
			}

			else if (os_strcmp(token, "electrodragon") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", sysCfg.electrodragon);
				else
				{
					sysCfg.electrodragon = intValue;
					doUpdate = 1; ok();
				}
			}

			else if (os_strcmp(token,"i2c_check")==0){
			int a,b;
			i2c_master_gpio_init();
			b=0;
			for (a=1; a<128; a++)
			if (i2c_check(a)) { ++b; iprintf(RESPONSE, "I2c device: %d (0x%02x) present\r\n", a,a); }
			iprintf(RESPONSE,"%d Device(s) detected\r\n",b);
			}

			else if (os_strcmp(token,"nano")==0){
			int a;
			uint8_t parm[2];
			i2c_master_gpio_init();
			if (argCount==4)
				{
				parm[0]=arg3;
				parm[1]=arg4;
				if (arg2==7) a=i2c_writeData(arg1,arg2,strValue,strlen(strValue)+1);
				else a=i2c_writeData(arg1,arg2,parm,2);
				ok();
				}
			else
				{
				a=i2c_readDataWithParam(arg1,arg2,arg3,parm,2);
				isQuery = 1;
				os_sprintf(strValue, "{nano_rval:%d}",(parm[0]<<8)+parm[1]);
				}
			}


			else if (os_strcmp(token,"ads1115")==0){
			int a;
			uint8_t parm[3];
			if (arg1==0)
				{
				isQuery = 1;
				a=i2c_general(0x48,parm,0,2); // just read 2 bytes - that's it
				os_sprintf(strValue, "%d",(parm[0]<<8)+parm[1]);
				}
			else if (arg1<5)
				{
				i2c_master_gpio_init();
				parm[0]=1;
				if (arg1==1) parm[1]=0xc2; // input A0
				if (arg1==2) parm[1]=0xd2; // input A1
				if (arg1==3) parm[1]=0xe2; // input A2
				if (arg1==4) parm[1]=0xf2; // input A3
				parm[2]=0x83;
				a=i2c_general(0x48,parm,3,0);
				parm[0]=0;
				a=i2c_general(0x48,parm,1,0);
				ok();
				}
			}

			// arg1=device, arg2=output bit(s), arg3=value. If arg2==0, set freq/setup
			else if (os_strcmp(token,"pca9685")==0){  //  so arg1 is chip address - if arg2 is 0, send freq else send on and off <4096
			int a;
			i2c_master_gpio_init();
			if (arg2==0) {
					pwmFrequency(arg1,arg3);
					for (a=0;a<16;a++) pwmSet(arg1,a,0,4095);
					}
			else
			if (arg2&1) pwmSet(arg1,0,0,4095-arg3);
			if (arg2&2) pwmSet(arg1,1,0,4095-arg3);
			if (arg2&4) pwmSet(arg1,2,0,4095-arg3);
			if (arg2&8) pwmSet(arg1,3,0,4095-arg3);
			if (arg2&0x10) pwmSet(arg1,4,0,4095-arg3);
			if (arg2&0x20) pwmSet(arg1,5,0,4095-arg3);
			if (arg2&0x40) pwmSet(arg1,6,0,4095-arg3);
			if (arg2&0x80) pwmSet(arg1,7,0,4095-arg3);
			if (arg2&0x100) pwmSet(arg1,8,0,4095-arg3);
			if (arg2&0x200) pwmSet(arg1,9,0,4095-arg3);
			if (arg2&0x400) pwmSet(arg1,10,0,4095-arg3);
			if (arg2&0x800) pwmSet(arg1,11,0,4095-arg3);
			if (arg2&0x1000) pwmSet(arg1,12,0,4095-arg3);
			if (arg2&0x2000) pwmSet(arg1,13,0,4095-arg3);
			if (arg2&0x4000) pwmSet(arg1,14,0,4095-arg3);
			if (arg2&0x8000) pwmSet(arg1,15,0,4095-arg3);
			ok();
			}

			else if (os_strcmp(token, "i2c") == 0) {
				if (argCount == 1)
				{

					isQuery = 1; i2c_master_gpio_init();
					i2c_master_start();
					i2c_master_writeByte((arg1 << 1) | 1); // send a read command
					if (!i2c_master_checkAck())
					{
						i2c_master_stop();
						os_sprintf(strValue, "Bad i2c");
					}
					else
					{
						uint8_t buff[40], a;
						a = i2c_master_readByte();
						i2c_master_stop();
						os_sprintf(strValue, "%d", a);
					}
				}
				else if (argCount >= 3)
				{
					i2c_master_gpio_init();
					i2c_master_start();
					i2c_master_writeByte(arg1 << 1);
					if (!i2c_master_checkAck())
					{
						i2c_master_stop();
						isQuery = 1;
						os_sprintf(strValue, "Bad i2c (1)");
					}
					else
					{
						i2c_master_writeByte((uint8_t)arg3);
						i2c_master_checkAck();
						if (argCount >= 4) {
							i2c_master_writeByte((uint8_t)arg4);
							i2c_master_checkAck();
						}
						if (argCount >= 5) {
							i2c_master_writeByte((uint8_t)arg5);
							i2c_master_checkAck();
						}
						if (argCount >= 6) {
							i2c_master_writeByte((uint8_t)arg6);
							i2c_master_checkAck();
						}
						i2c_master_stop();
						if (arg2 == 1)
						{
							isQuery = 1;
							i2c_master_start();
							i2c_master_writeByte((arg1 << 1) | 1);
							if (!i2c_master_checkAck())
							{
								i2c_master_stop();
								os_sprintf(strValue, "duff i2c (2)");
							}
							else
							{
								uint8_t a;
								a = i2c_master_readByte();
								i2c_master_stop();
								os_sprintf(strValue, "%d", a);
							}
						} else ok();
					}
				}
				else { isQuery = 1; os_sprintf(strValue, "Bad arguments"); }
			}

			// 2 line and 4 line LCDs
			// {hitachi:39,"There"}  where 39 is device number - using a separate i2c expander
			//$s setup - $c clear  $ 1..4 line 1-4 $t time $d date

			else if (os_strcmp(token, "hitachi") == 0) {

				if (argCount >= 2)
				{
					if (arg1 != 255)
					{
						lcd_parallel = 0;
						i2c_master_gpio_init();
						i2c_master_start();
						i2c_master_writeByte(arg1 << 1);
					}
					else lcd_parallel = 1;
					if ((arg1 != 255) && (!i2c_master_checkAck()))
					{
						i2c_master_stop();
						isQuery = 1; os_sprintf(strValue, "Bad i2c");
					}
					else
					{
						uint8_t *gg;
						char tmb[12];
						gg = strValue;
						while (*gg)
						{
							if (*gg == '$') {
								gg++;
								switch (*gg++)
								{
								case 0   :  gg--; continue;
								case 'o' :  hitachiHiLo(0xdf, 1); continue; // degrees
								case 'b' :  hitachiHiLo(0xff, 1); continue; // full block
								case 'r' :  hitachiHiLo(0x7e, 1); continue; // right arrow
								case 'l' :  hitachiHiLo(0x7f, 1); continue; // left arrow
								case 'x' :  hitachiHiLoS((hitachi_offset == 8) ? "        " : "          ", 1); continue;
								case 'X' :  hitachiHiLoS("                    ", 1); continue;
								case 'c' :  hitachiHiLo(1, 0); os_delay_us(2000); continue;

								case 'S' :  hitachi_line_3 = 148; // 20*4 LCD
									hitachi_line_4 = 212; // 20*4 display
									hitachi_offset = 10;
									goto initLCD;

								case 's' :  hitachi_line_3 = 144; // 16x4 LCD
									hitachi_line_4 = 208; // 16x4 LCD
									hitachi_offset = 8;
initLCD:	  						hitachiByte(0x30, 0); os_delay_us(38);
									hitachiByte(0x30, 0); os_delay_us(38);
									hitachiByte(0x30, 0); os_delay_us(38);
									hitachiByte(0x20, 0); os_delay_us(38);
									hitachiHiLo(8, 0); os_delay_us(38);
									hitachiHiLo(0x28, 0); os_delay_us(38);
									hitachiHiLo(1, 0); os_delay_us(2000);
									hitachiHiLo(0x0c, 0); os_delay_us(38);
									continue;
								case '1' :  hitachiHiLo(128, 0); os_delay_us(38); continue;
								case '2' :  hitachiHiLo(192, 0); os_delay_us(38); continue;
								case '3' :  hitachiHiLo(hitachi_line_3, 0); os_delay_us(38); continue;
								case '4' :  hitachiHiLo(hitachi_line_4, 0); os_delay_us(38); continue;
								case '5' :  hitachiHiLo(128 + hitachi_offset, 0); os_delay_us(38); continue;
								case '6' :  hitachiHiLo(192 + hitachi_offset, 0); os_delay_us(38); continue;
								case '7' :  hitachiHiLo(hitachi_line_3 + hitachi_offset, 0); os_delay_us(38); continue;
								case '8' :  hitachiHiLo(hitachi_line_4 + hitachi_offset, 0); os_delay_us(38); continue;

								case 't' :  os_sprintf(tmb, "%02d:%02d:%02d", tm.Hour, tm.Minute, tm.Second);
									hitachiHiLoS(tmb, 1); continue;
								case 'd' :  os_sprintf(tmb, "%02d/%02d/%02d", tm.Day, tm.Month, tm.Year);
									hitachiHiLoS(tmb, 1); continue;

								case 'p' :  os_sprintf(tmb, "%02d", temperature);
									hitachiHiLoS(tmb, 1); continue;

								case 'h' :  os_sprintf(tmb, "%02d", humidity);
									hitachiHiLoS(tmb, 1); continue;

								case 'i' :  wifi_get_ip_info(0, &theIp);
									os_sprintf(tmb, "%d:%d:%d:%d", theIp.ip.addr & 255, (theIp.ip.addr >> 8) & 255, (theIp.ip.addr >> 16) & 255, theIp.ip.addr >> 24);
									hitachiHiLoS(tmb, 1); continue;
								case 'f' :  lcdBacklightStatus = 0; i2c_master_writeByte(lcdBacklightStatus); i2c_master_checkAck(); continue;
								case 'F' :  lcdBacklightStatus = LCD_BL_PIN; i2c_master_writeByte(lcdBacklightStatus); i2c_master_checkAck(); continue;
								}
							} // or drop through and display - like $$ for example
							hitachiHiLo(*gg++, 1);
						}
						if (arg1 != 255) i2c_master_stop();
						ok();

					}
				}
				else { isQuery = 1; os_sprintf(strValue, "Bad arguments"); }
			}

			else if (os_strcmp(token, "oled") == 0) { // untested

				extern char ScreenData[3][4][21];

				i2c_master_gpio_init();    // init i2c (RTC, OLED)
				i2c_master_init();
				display_init();            // OLED
				os_sprintf(&ScreenData[0][0][0], "%s", "Temperature");
				os_sprintf(&ScreenData[1][0][0], "%s", "Time, messages");
				os_sprintf(&ScreenData[2][0][0], "%s", "Network");

				display_clear();
				display_on();
				ok();
			}


			else if (os_strcmp(token, "gy30_init") ==0) {
				if (argCount==1)
				{
					i2c_master_gpio_init();    // init i2c (RTC, OLED)
					i2c_master_init();
					uint8_t lt[1];
					lt[0]=0;
					i2c_writeData(arg1,0x10,lt,0);
					isQuery=0; ok();
				}
			}


			else if (os_strcmp(token, "gy30_read") ==0) {
				if (argCount==1)
				{
					i2c_master_init();
					uint8_t lt[2];
					i2c_readData(arg1,-1,lt,2);
					os_sprintf(strValue,"%d",((uint16_t)lt[0])*256+lt[1]);
					isQuery=1;
				}
			}

			else if (os_strcmp(token, "seeed") == 0) {
				system_soft_wdt_stop();
				uint8_t *gg;
				char tmb[12];
				gg = strValue;
				while (*gg)
				{
					if (*gg == '$') {
						gg++;
						switch (*gg++)
						{
						case 0   :  gg--; continue;
						case 'o' :  seeedPutChar(0xdf); continue; // degrees
						case 'b' :  seeedPutChar(0xff); continue; // full block
						case 'r' :  seeedPutChar(0x7e); continue; // right arrow
						case 'l' :  seeedPutChar(0x7f); continue; // left arrow
						case 'x' :  seeedPutString("                "); continue;
						case 'c' :  seeedSetTextXY(0, 0); seeedSendClear(0, 48 * 96); seeedSetTextXY(0, 0); continue;
						case 's' :  seeedInit();
							seeedSendClear(0, 48 * 96);
							seeedSetNormalDisplay();
							seeedSetVerticalMode();
							seeedSetTextXY(0, 0);
							seeedSetGreyLevel(6);
							continue;
						case 'g' :  if (*(gg + 1) == '0') {gg += 2; seeedSetGreyLevel(10); }
							else if (*(gg + 1) == '1') {gg += 2; seeedSetGreyLevel(11); }
							else if (*(gg + 1) == '2') {gg += 2; seeedSetGreyLevel(12); }
							else if (*(gg + 1) == '3') {gg += 2; seeedSetGreyLevel(13); }
							else if (*(gg + 1) == '4') {gg += 2; seeedSetGreyLevel(14); }
							else if (*(gg + 1) == '5') {gg += 2; seeedSetGreyLevel(15); }
							else { seeedSetGreyLevel((*gg) - ' '); gg++; }
							continue;
						case '1' :  if (*gg == '0') {gg++; seeedSetTextXY(9, 0); }
							else if (*gg == '1') {gg++; seeedSetTextXY(10, 0); }
							else if (*gg == '2') {gg++; seeedSetTextXY(11, 0); }
							else seeedSetTextXY(0, 0);
							continue;
						case '2' :  seeedSetTextXY(1, 0); continue;
						case '3' :  seeedSetTextXY(2, 0); continue;
						case '4' :  seeedSetTextXY(3, 0); continue;
						case '5' :  seeedSetTextXY(4, 0); continue;
						case '6' :  seeedSetTextXY(5, 0); continue;
						case '7' :  seeedSetTextXY(6, 0); continue;
						case '8' :  seeedSetTextXY(7, 0); continue;
						case '9' :  seeedSetTextXY(8, 0); continue;

						case 't' :  os_sprintf(tmb, "%02d:%02d:%02d", tm.Hour, tm.Minute, tm.Second);
							seeedPutString(tmb); continue;
						case 'd' :  os_sprintf(tmb, "%02d/%02d/%02d", tm.Day, tm.Month, tm.Year);
							seeedPutString(tmb); continue;
						case 'i' :  wifi_get_ip_info(0, &theIp);
							os_sprintf(tmb, "%d:%d:%d:%d", theIp.ip.addr & 255, (theIp.ip.addr >> 8) & 255, (theIp.ip.addr >> 16) & 255, theIp.ip.addr >> 24);
							seeedPutString(tmb); continue;

						case 'p' :  os_sprintf(tmb, "%02d", temperature);
							seeedPutString(tmb); continue;

						case 'h' :  os_sprintf(tmb, "%02d", humidity);
							seeedPutString(tmb); continue;
						}
					} // or drop through and display - like $$ for example
					seeedPutChar(*gg++);
				}
				system_soft_wdt_restart();
				ok();
			}

			else if (os_strcmp(token, "spi_init") == 0) {
				sysCfg.enable13 = 1; // stop 13 use as an indicator
				spiActive = 1; // Can't be doing stuff on 14 when using SPI! Also make sure temperature port is not 14 !!
				spi_init(1); // HSPI mode
				spi_clock(1,2,2); // 0,0 would not operate for displays so for now assume slower spi - this works
				ok();
				spiActive = 0;
			}

			// some commands here for an experimental MQTT VT-100 type terminal which can come up at powerup
			// resets automatically
			else if (os_strcmp(token, "ili_set") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", sysCfg.iliGo);
				else
				{
					if (intValue==1) { iliStartup(); sysCfg.iliGo=45;  }
					else sysCfg.iliGo=0;
					doUpdate = 1;
					ok();
				}
			}

			else if (os_strcmp(token, "ili_c") == 0) {  // experimental - init ILI 240*320 display as VT100
			int x;
			if (sysCfg.iliGo==45)  { ili9340_setFrontColor(0xFFFF);  ili9340_drawChar(80,80,'A');  ili9340_drawBigString(10,100,"Hello there how are you"); }

			for (x=0;x<200;x++)
			ili9340_drawPixel(x,100,0xffff);
			ok();
			}

			else if (os_strcmp(token, "ili_test") == 0) {  // experimental - init ILI 240*320 display as VT100

			uint8_t blg1,blg2,blg3,blg4;
			blg1=read_rom_uint8(daft);
			blg2=read_rom_uint8(daft+1);
			blg3=read_rom_uint8(daft+2);
			blg4=read_rom_uint8(daft+3);
			iprintf(RESPONSE, "Blah %02x %02x %02x %02x\r\n", (uint16_t)blg1,(uint16_t)blg2,(uint16_t)blg3,(uint16_t)blg4);
			uint16_t ablg1,ablg2,ablg3,ablg4;
			ablg1=read_rom_uint16(daft);
			ablg2=read_rom_uint16(daft+2);
			ablg3=read_rom_uint16(daft+4);
			ablg4=read_rom_uint16(daft+6);
			iprintf(RESPONSE, "Blah %02x %02x %02x %02x\r\n", (uint16_t)ablg1,(uint16_t)ablg2,(uint16_t)ablg3,(uint16_t)ablg4);
			}

			else if (os_strcmp(token, "ili_reset") == 0) {  // experimental - init ILI 240*320 display as VT100
			if (sysCfg.iliGo==45) iliStartup();
			  ok();
			}

			else if (os_strcmp(token, "ili_text") == 0) {  // experimental send string
			  if (sysCfg.iliGo==45) vt100_puts(strValue);
			}

			else if (os_strcmp(token, "ili_status") == 0) {  // experimental update status
			if (sysCfg.iliGo==45)
			  {
			  vt100_puts("\e7\e[39;1H\e[K"); // store cursor, position cursor, erase to end of line
			  vt100_puts("\e[35;40m"); // purple on black
			  vt100_puts(strValue);
			  vt100_puts("\e8"); // restore cursor
			  }
			}

			else if (os_strcmp(token, "ili_rssi") == 0) {  // experimental update status(right) with rssi
			if (sysCfg.iliGo==45)
			{
			  vt100_puts("\e7\e[39;1H"); // store cursor, position cursor, erase to end of line
			  vt100_puts("\e[35;40m"); // purple on black
			  int x=wifi_station_get_rssi();
			  os_sprintf(strValue,"%02d:%02d %02d/%02d/%02d",tm.Hour,tm.Minute,tm.Day,tm.Month,tm.Year); vt100_puts(strValue);
			   vt100_puts("\e8"); // restore cursor
			  // Colour definitions
			  #define	ILI9340_BLACK   0x0000
			  #define	ILI9340_BLUE    0x001F
			  #define	ILI9340_RED     0xF800
			  #define	ILI9340_GREEN   0x07E0
			  #define ILI9340_CYAN    0x07FF
			  #define ILI9340_MAGENTA 0xF81F
			  #define ILI9340_YELLOW  0xFFE0
			  #define ILI9340_WHITE   0xFFFF
			  x=-x;
			  if (x<90) ili9340_fillRect(218,312,3,2,ILI9340_RED); else ili9340_fillRect(218,312,3,2,ILI9340_BLUE);
			  if (x<80) ili9340_fillRect(222,310,3,4,ILI9340_RED); else ili9340_fillRect(222,310,3,4,ILI9340_BLUE);
			  if (x<70) ili9340_fillRect(226,308,3,6,ILI9340_RED); else ili9340_fillRect(226,308,3,6,ILI9340_BLUE);
			  if (x<60) ili9340_fillRect(230,306,3,8,ILI9340_RED); else ili9340_fillRect(230,306,3,8,ILI9340_BLUE);
			  if (x<50) ili9340_fillRect(234,304,3,10,ILI9340_RED); else ili9340_fillRect(234,304,3,10,ILI9340_BLUE);
			 }
			}

			else if (os_strcmp(token, "ili_title") == 0) {  // experiment update status
			if (sysCfg.iliGo==45)
			  {
			  vt100_puts("\e7\e[2;31H\e[1K\e[2;1H"); // store cursor, position cursor, erase to start of line missing the LEDS
			  vt100_puts("\e[35;40m"); // purple on black
			  vt100_puts(strValue);
			  vt100_puts("\e8"); // restore cursor
			  }
			}

			else if (os_strcmp(token, "in14") == 0) {
				os_sprintf(strValue, "%d", in14Value);
			}

			else if (os_strcmp(token, "in14_count") == 0) {
				os_sprintf(strValue, "%lu", in14Count);
				in14Count = 0;
			}

			else if (os_strcmp(token, "in2_count") == 0) {
				os_sprintf(strValue, "%lu", in2Count);
				in2Count = 0;
			}

			else if (os_strcmp(token, "cpu_freq") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", system_get_cpu_freq());
				else
				{
					if ((intValue == 80) || (intValue == 160)) { sysCfg.system_clock = intValue; system_update_cpu_freq(intValue); doUpdate = 1; }
					ok();
				}
			}

			else if (os_strcmp(token, "rgb") == 0)
			{
				if (arg1 == 4) arg1 = 5; else if (arg1 == 5) arg1 = 4;
				wsBitMask = (uint8_t) arg1;
				rgb.red = arg3;
				rgb.green = arg2;
				rgb.blue = arg4;
				if (arg5 == -1) arg5 = 10;
				if (arg6 == -1) arg6 = 4;
				rgb.rgbnum = arg5;
				rgb.rgbdelay = arg6;
				rgbOnce = 1;
				ok();
			}

			else if (os_strcmp(token, "rgbperm") == 0)
			{
				if (arg1 == 4) arg1 = 5; else if (arg1 == 5) arg1 = 4;
				if ((arg1 <= 16) && (arg1 >= 0))
				{
					wsBitMask = (uint8_t) arg1;
					rgb.red = arg3;
					rgb.green = arg2;
					rgb.blue = arg4;
					if (arg5 == -1) arg5 = 10;
					if (arg6 == -1) arg6 = 4;
					rgb.rgbnum = arg5;
					rgb.rgbdelay = arg6;
					rgbOnce = 1;
				}
				sysCfg.rgbPermPort = arg1;
				sysCfg.rgbPermRed = arg2;
				sysCfg.rgbPermGreen = arg3;
				sysCfg.rgbPermBlue = arg4;
				sysCfg.rgbPermNum = arg5;
				doUpdate = 1; // permanently store this RGB setting - so that next powerup will repeat unless pin is -1
				ok();
			}

			else if (os_strcmp(token, "kelvin") == 0)
			{
				if (arg1 == 4) arg1 = 5; else if (arg1 == 5) arg1 = 4;
				wsBitMask = (uint8_t) arg1;

				uint16 kel;
				kel = arg2;
				if (kel < 1000) kel = 1000;
				if (kel > 40000) kel = 40000;
				kel -= 1000;
				kel /= 500;
				kel *= 3;

				if (arg2 == 0)
				{
					rgb.green = 0;
					rgb.red = 0;
					rgb.blue = 0;
				}
				else
				{
					rgb.green = read_rom_uint8(kelvin + kel);
					rgb.red = read_rom_uint8(kelvin + kel + 1);
					rgb.blue = read_rom_uint8(kelvin + kel + 2);
				}

				if (arg3 == -1) arg3 = 10;
				if (arg4 == -1) arg4 = 2;
				rgb.rgbnum = arg3;
				rgb.rgbdelay = arg4;
				rgbOnce = 1;
				ok();
			}

			else if (os_strcmp(token, "hsv") == 0)
			{
				struct RGB_set RGB;
				struct HSV_set HSV;
				if (arg1 == 4) arg1 = 5; else if (arg1 == 5) arg1 = 4;
				wsBitMask = (uint8_t) arg1;
				HSV.h = arg2;
				HSV.s = arg3;
				HSV.v = arg4;
				HSV2RGB(HSV, &RGB);
				rgb.red = RGB.g;  // don't worry - ws just not right way...
				rgb.green = RGB.r;
				rgb.blue = RGB.b;
				if (arg5 == -1) arg5 = 10;
				if (arg6 == -1) arg6 = 2;
				rgb.rgbnum = arg5;
				rgb.rgbdelay = arg6;
				rgbOnce = 1;
				ok();
			}

			else if (os_strcmp(token, "rgbgo") == 0)
			{
				if (arg1 == 4) arg1 = 5; else if (arg1 == 5) arg1 = 4;
				wsBitMask = (uint8_t) arg1;
				if (arg2 > 300) arg2 = 300;
				WS2812OutBuffer(rgb.buffer, arg2 * 3, 1);   // 3 leds in array, number of repetitions
				ok();
			}

			else if (os_strcmp(token, "rgbset") == 0)
			{
				int a;
				if ((arg1 + arg2) > 300) iprintf(RESPONSE, "Bad numbers\r\n");
				else
				{
					a = arg1 * 3;
					while (a < ((arg1 * 3) + (arg2 * 3)))
					{
						rgb.buffer[a++] = arg4;
						rgb.buffer[a++] = arg3;
						rgb.buffer[a++] = arg5;
					}
					ok();
				}
			}

			else if (os_strcmp(token, "rainbow") == 0)
			{	// number of leds - and the number of repetitions
				if (arg1 == 4) arg1 = 5; else if (arg1 == 5) arg1 = 4;
				wsBitMask = (uint8_t) arg1;
				rgb.rgbnum = arg2;
				rgb.rainbow = arg3;
				if (arg4 < 5) arg4 = 5;
				if (argCount > 3) os_timer_arm(&ledTimer, arg4, 1);
				ok();
			}

			else if (os_strcmp(token, "pwm") == 0) {
				pwm_array.red = arg2;
				pwm_array.green = arg1;
				pwm_array.blue = arg3;
				if (donepwm == 0) {
					pwm_init(freq, duty, 3, io_info);
					donepwm = 1;
				}
				if ((argCount == 4) && (arg4 == 0))
				{
					pwm_array.reda = pwm_array.red; pwm_array.greena = pwm_array.green; pwm_array.bluea = pwm_array.blue;
					pwm_set_duty((uint32_t)PWMTable[pwm_array.red], (uint8)0);
					pwm_set_duty((uint32_t)PWMTable[pwm_array.green], (uint8)1);
					pwm_set_duty((uint32_t)PWMTable[pwm_array.blue], (uint8)2);
					pwm_start();
				}
				else
				{
					if (argCount < 4) arg4 = 20;
					os_timer_arm(&ledTimer, arg4, 1);
				}
				ok();
			}

			else if (os_strcmp(token, "pwmhsv") == 0) {
				struct RGB_set RGB;
				struct HSV_set HSV;
				HSV.h = arg1;
				HSV.s = arg2;
				HSV.v = arg3;
				HSV2RGB(HSV, &RGB);
				pwm_array.red = RGB.g;
				pwm_array.green = RGB.r;
				pwm_array.blue = RGB.b;
				if (donepwm == 0) {
					pwm_init(freq, duty, 4, io_info);
					donepwm = 1;
				}
				if  ((argCount == 4) && (arg4 == 0))
				{
					pwm_array.reda = pwm_array.red; pwm_array.greena = pwm_array.green; pwm_array.bluea = pwm_array.blue;
					pwm_set_duty((uint32_t)PWMTable[pwm_array.red], (uint8)0);
					pwm_set_duty((uint32_t)PWMTable[pwm_array.green], (uint8)1);
					pwm_set_duty((uint32_t)PWMTable[pwm_array.blue], (uint8)2);
					pwm_start();
				}
				else
				{
					if (argCount < 4) arg4 = 20;
					os_timer_arm(&ledTimer, arg4, 1);
				}
				ok();
			}


			else if (os_strcmp(token, "rgbstop") == 0) {
				rgbPlayBuffer[0] = -1; rgbPlayPtr = 0; rgbRecordPtr = 0; rgbDoSeq = 0;
				ok();
			}

			else if (os_strcmp(token, "mqtt") == 0)
			{
				if (os_strlen(strValue) && os_strlen(strValue2))
					MQTT_Publish(&mqttClient, strValue, strValue2, os_strlen(strValue2), 0, 0);
			}

			else if (os_strcmp(token, "rgbstart") == 0) {
				if (arg1 == 4) arg1 = 5; else if (arg1 == 5) arg1 = 4;

				wsBitMask = (uint8_t) arg1;
				rgbPlayPtr = 0;
				rgbRecordPtr = 0;
				rgbDoSeq = 1; rgbTotal = arg2;
				rgbPlayBuffer[0] = -1;
				if (rgbTotal > 300) rgbTotal = 300;
				int a = 0;
				while (a < (rgbTotal * 3))
				{
					rgb.buffer[a++] = 0;
				}
				WS2812OutBuffer(rgb.buffer, rgbTotal * 3, 1);
				ok();
			}

			else if (os_strcmp(token, "rgbadd") == 0) {
				if ((rgbRecordPtr < (RGBMAX - 7)) && ((arg1 + arg2) <= rgbTotal))
				{
					rgbPlayBuffer[rgbRecordPtr++] = arg1;
					rgbPlayBuffer[rgbRecordPtr++] = arg2;
					rgbPlayBuffer[rgbRecordPtr++] = arg3;
					rgbPlayBuffer[rgbRecordPtr++] = arg4;
					rgbPlayBuffer[rgbRecordPtr++] = arg5;
					rgbPlayBuffer[rgbRecordPtr++] = arg6;
					rgbPlayBuffer[rgbRecordPtr] = -1;
					ok();
				}
				else if ((arg1 + arg2) > rgbTotal)iprintf(RESPONSE, "Beyond strip maximum\r\n"); else iprintf(RESPONSE, "RGB buffer full\r\n");
			}

			else if (os_strcmp(token, "out0") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", (sysCfg.out0Status == 0) ? "OFF" : "ON");
				else {
					if (arg1 == 6) { if (arg2 > 10002) out0Timer = arg2 - 10000; else out0Timer = arg2 * 60; }  else out0Timer=0;
					if (arg1==-1) { if (sysCfg.out0Status) arg1=0; else arg1=1; } // toggle
					if (sysCfg.out0Status != arg1) {
						sysCfg.out0Status = arg1;
						doUpdate = 1;
					}
					if (sysCfg.out0Status != 6)
					{
						(sysCfg.invert & 1) ? easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.out0Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.out0Status == 1) ? OUT_ON : OUT_OFF));
					}
					ok();
				}
			}

			else if (os_strcmp(token, "out2") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", (sysCfg.out2Status == 0) ? "OFF" : "ON");
				else
				{
					if (arg1 == 6) { if (arg2 > 10002) out2Timer = arg2 - 10000; else out2Timer = arg2 * 60; }  else out2Timer=0;
					if (arg1==-1) { if (sysCfg.out2Status) arg1=0; else arg1=1; } // toggle
					if (sysCfg.out2Status != arg1) {
						sysCfg.out2Status = arg1;
						doUpdate = 1;
					}
					if (sysCfg.out2Status != 6)
					{
						(sysCfg.invert & 2) ? easygpio_outputSet(2, ((sysCfg.out2Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(2, ((sysCfg.out2Status == 1) ? OUT_ON : OUT_OFF));
					}
					ok();
				}
			}

			else if ((os_strcmp(token, "out4") == 0) && (sysCfg.serial2 == 0)) { // - chip pins reversed
				if (isQuery) os_sprintf(strValue, "%s", (sysCfg.out5Status == 0) ? "OFF" : "ON");
				else {
					if (arg1 == 6) { if (arg2 > 10002) out5Timer = arg2 - 10000; else out5Timer = arg2 * 60; }  else out5Timer=0;
					if (arg1==-1) { if (sysCfg.out5Status) arg1=0; else arg1=1; } // toggle
					if (sysCfg.out5Status != arg1) {
						sysCfg.out5Status = arg1;
						doUpdate = 1;
					}
					if (sysCfg.out5Status != 6)
					{
						(sysCfg.invert & 4) ? easygpio_outputSet(5, ((sysCfg.out5Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(5, ((sysCfg.out5Status == 1) ? OUT_ON : OUT_OFF));
					}
					ok();
				}
			}

			else if ((os_strcmp(token, "out5") == 0) && (sysCfg.serial2 == 0)) { // - chip pins reversed
				if (isQuery) os_sprintf(strValue, "%s", (sysCfg.out4Status == 0) ? "OFF" : "ON");
				else
				{
					if (arg1 == 6) { if (arg2 > 10002) out4Timer = arg2 - 10000; else out4Timer = arg2 * 60; }  else out4Timer=0;
					if (arg1==-1) { if (sysCfg.out4Status) arg1=0; else arg1=1; } // toggle
					if (sysCfg.out4Status != arg1) {
						sysCfg.out4Status = arg1;

						doUpdate = 1;
					}
					if (sysCfg.out4Status != 6)
					{
						(sysCfg.invert & 8) ? easygpio_outputSet(4, ((sysCfg.out4Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(4, ((sysCfg.out4Status == 1) ? OUT_ON : OUT_OFF));
					}
					ok();
				}
			}

			else if (os_strcmp(token, "out12") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", (sysCfg.out12Status == 0) ? "OFF" : "ON");
				else {
					if (arg1 == 6) { if (arg2 > 10002) out12Timer = arg2 - 10000; else out12Timer = arg2 * 60; } else out12Timer=0;
					if (arg1==-1) { if (sysCfg.out12Status) arg1=0; else arg1=1; } // toggle
					if (sysCfg.out12Status != arg1) {
						sysCfg.out12Status = arg1;
						doUpdate = 1;
					}
					if (sysCfg.out12Status != 6)
					{
						(sysCfg.invert & 16) ? easygpio_outputSet(12, ((sysCfg.out12Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(12, ((sysCfg.out12Status == 1) ? OUT_ON : OUT_OFF));
					}
					ok();
				}
			}

			else if (os_strcmp(token, "out13") == 0)  {
				if (isQuery) os_sprintf(strValue, "%s", (sysCfg.out13Status == 0) ? "OFF" : "ON");
				else {
					if (arg1 == 6) { if (arg2 > 10002) out13Timer = arg2 - 10000; else out13Timer = arg2 * 60; }  else out13Timer=0;
					if (arg1==-1) { if (sysCfg.out13Status) arg1=0; else arg1=1; } // toggle
					if (sysCfg.out13Status != arg1) {
						sysCfg.out13Status = arg1;
						doUpdate = 1;
					}
					if (sysCfg.out13Status != 6)
					{
						(sysCfg.invert & 32) ? easygpio_outputSet(13, ((sysCfg.out13Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(13, ((sysCfg.out13Status == 1) ? OUT_ON : OUT_OFF));
					}
					ok();
				}
			}


			// enable GPIO14 output.
			else if (os_strcmp(token, "enable14") == 0) { // nde (15-11-16) enable 14
				if (sysCfg.out14Enable != intValue) {
				   sysCfg.out14Enable = intValue;
				   doUpdate = 1;
				}

				if (intValue== 1){                                    // nde (15-11-16) enable 14
				  easygpio_pinMode(14, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
				}
				else {
				   easygpio_pinMode(14, EASYGPIO_NOPULL, EASYGPIO_INPUT);
				}
				ok();
			}

			// switch GPIO14 state
			else if (os_strcmp(token, "out14") == 0)  {
				if (isQuery) os_sprintf(strValue, "%s", (sysCfg.out14Status == 0) ? "OFF" : "ON");
				else {
				   if (arg1 == 6) {
					 if (arg2 > 10002) out14Timer = arg2 - 10000; else out14Timer = arg2 * 60; }
					 else out14Timer=0;
				   if (sysCfg.out14Status != arg1) {
					 sysCfg.out14Status = arg1;
					 doUpdate = 1;
					 }
				   if (sysCfg.out14Status != 6) {
					(sysCfg.newInvert & 1) ? easygpio_outputSet(14, ((sysCfg.out14Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(14,   ((sysCfg.out14Status == 1) ? OUT_ON : OUT_OFF));
				   }
				   ok();
				}
			}




			else if (os_strcmp(token, "out15") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", (sysCfg.out15Status == 0) ? "OFF" : "ON");
				else {
					if (arg1 == 6) { if (arg2 > 10002) out15Timer = arg2 -= 10000; else out15Timer = arg2 * 60; }  else out15Timer=0;
					if (arg1==-1) { if (sysCfg.out15Status) arg1=0; else arg1=1; } // toggle
					if (sysCfg.out15Status != arg1) {
						sysCfg.out15Status = arg1;
						doUpdate = 1;
					}
					if (sysCfg.out15Status != 6)
					{
						(sysCfg.invert & 64) ? easygpio_outputSet(15, ((sysCfg.out15Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(15, ((sysCfg.out15Status == 1) ? OUT_ON : OUT_OFF));
					}
					ok();
				}
			}

			else if (os_strcmp(token, "out16") == 0) {
				if (isQuery) os_sprintf(strValue, "%s", (sysCfg.out16Status == 0) ? "OFF" : "ON");
				else {
					if (arg1 == 6) { if (arg2 > 10002) out16Timer = arg2 - 10000; else out16Timer = arg2 * 60; }  else out16Timer=0;
					if (arg1==-1) { if (sysCfg.out16Status) arg1=0; else arg1=1; } // toggle
					if (sysCfg.out16Status != arg1) {
						sysCfg.out16Status = arg1;
						doUpdate = 1;
					}
					if (sysCfg.out16Status != 6)
					{
						(sysCfg.invert & 128) ? easygpio_outputSet(16, ((sysCfg.out16Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(16, ((sysCfg.out16Status == 1) ? OUT_ON : OUT_OFF));
					}
					ok();
				}
			}


			else if (os_strcmp(token, "xport") == 0) {
					if (arg1<64)
					{

					if (argCount==1) { isQuery=1; os_sprintf(strValue, "%d", moreInPorts(39-(arg1/8),arg1/8,(arg1&7))); }
					else
						{
						if (argCount>=2) { morePorts(39-(arg1/8),arg1/8,(arg1&7),arg2); }
						ok();
						}
					}
			}


			else if (os_strcmp(token, "flashsize") == 0) {
				if (isQuery) {
					uint32_t id = spi_flash_get_id();
					uint8_t mfgr_id = id & 0xff;
					uint8_t type_id = (id >> 8) & 0xff; // not relevant for size calculation
					uint8_t size_id = (id >> 16) & 0xff; // lucky for us, WinBond ID's their chips in a form that lets us calculate the size
					if (mfgr_id != 0xEF) // 0xEF is WinBond; that's all we care about (for now)
						os_sprintf(strValue, "NOT WINBOND");
					else
						os_sprintf(strValue, "Manuf ID= %d, Type ID= %d, Size=%ld\r\n", (uint16_t) mfgr_id, (uint16_t) type_id, 1 << size_id );
				}
				else ok();
			}

			else if (os_strcmp(token, "reset") == 0) {
				cfgSave();
				system_restart();
			}

			else if (os_strcmp(token, "reset_config") == 0) {
				if (*topic==0) { cfgLoad(1); system_restart(); }
			}

			else if (os_strcmp(token, "clock") == 0) {
				if (isQuery) os_sprintf(strValue, "%d", sysCfg.clock);
				else {
					if (sysCfg.clock != 255)
					{
						int a;
						for (a = 0; a < 180; a += 3)
						{
							rgb.buffer[a] = 0;
							rgb.buffer[a + 1] = 0;
							rgb.buffer[a + 2] = 0;
						}
						WS2812OutBuffer(rgb.buffer, 180, 1); // 60 LEDs
					}
					if (intValue == 4) intValue = 5; else if (intValue == 5) intValue = 4; // usual swap for bad labelling
					sysCfg.clock = intValue;
					doUpdate = 1;
					ok();
				}
			}

			else if (os_strcmp(token, "in_2bounce") == 0) {
				sysCfg.in2Bounce = intValue;
				doUpdate = 1; ok();
			}

			else if (os_strcmp(token, "in_14bounce") == 0) {
				sysCfg.in14Bounce = intValue;
				doUpdate = 1; ok();
			}

			else if (os_strcmp(token, "reconnectMQTT") == 0) {
				MQTT_Connect(&mqttClient);
				ok();
			}

			else if (os_strcmp(token, "otaupdate") == 0) {
				isQuery = 1;
				if (system_get_flash_size_map() >= FLASH_SIZE_8M_MAP_512_512) {
					//os_timer_disarm(&lostThePlotTimer);
					//os_timer_disarm(&bounceTimer);
					//os_timer_disarm(&startupTimer);
					//os_timer_disarm(&temperatureTimer);
					//os_timer_disarm(&rtcTimer);
					//os_timer_disarm(&rgbTimer);
					//os_timer_disarm(&ledTimer);
					//os_timer_disarm(&clockTimer);
					OtaUpdate();
					os_sprintf(strValue, "%s", "Attempting OTA");
				} else {
					os_sprintf(strValue, "%s", "Not supported");
				}
			}

			else if (os_strcmp(token, "romswitch") == 0) {
				if (system_get_flash_size_map() >= FLASH_SIZE_8M_MAP_512_512) {
					uint8 before, after;
					before = rboot_get_current_rom();
					if (before == 0) after = 1; else after = 0;
					iprintf(INFO, "Swapping from rom %d to rom %d.\r\n", before, after);
					rboot_set_current_rom(after);
					iprintf(INFO, "Restarting...\r\n\r\n");
					system_restart();
				} else {
					iprintf(RESPONSE, "Not supported on this device\r\n");
				}
			}

			else if (os_strcmp(token, "override") == 0) {
				sysCfg.override0 = intValue;
				if (intValue == 0) easygpio_outputSet(0, ((sysCfg.out0Status == 1) ? OUT_ON : OUT_OFF));   //restore
				doUpdate = 1; ok();
			}

			else if (os_strcmp(token, "override_time") == 0) {
				sysCfg.override0Time = (uint32_t) intValue * 60 * 20;
				doUpdate = 1; ok();
			}

			else if (os_strcmp(token, "gettime") == 0) {
				uint32 current_stamp;
				isQuery = 1;
				current_stamp = sntp_get_current_timestamp();
				if (current_stamp) current_stamp -= (60 * 60 * 7);
				os_sprintf(strValue, "International time is %s \r\n", sntp_get_real_time(current_stamp));
			}

			else if (os_strcmp(token, "rgbinstant") == 0)
			{
				if (argCount == 4) rgblight(arg1, arg2, arg3, arg4);
				if (argCount == 5) { if (arg5 == 0) arg5 = 1; rgblights(arg1, arg2, arg3, arg4, arg5); }
				ok();
			}

			else if (os_strcmp(token, "timeout_set") == 0) {
				sysCfg.timeoutPeriod = arg1;
				timeoutCount = arg1;
				sysCfg.timeoutSecs = arg2;
				sysCfg.timeoutRepeat = arg3;
				if (arg4 == 4) arg4 = 5; else if (arg4 == 5) arg4 = 4;
				sysCfg.timeoutPin = arg4;
				sysCfg.timeoutInvert = arg5;
				easygpio_outputSet(sysCfg.timeoutPin, (sysCfg.timeoutInvert) ? OUT_ON : OUT_OFF);
				doUpdate = 1; ok();
			}

			else if (os_strcmp(token, "timeout_clear") == 0) {
				timeoutCount = sysCfg.timeoutPeriod;
				easygpio_outputSet(sysCfg.timeoutPin, (sysCfg.timeoutInvert) ? OUT_ON : OUT_OFF);
				doUpdate = 1; ok();
			}


			else if (os_strcmp(token,"quick")==0)
				{
					isQuery=1; os_sprintf(strValue, "Timer is %ld \r\n", quick_time);
				}

			else {
				iprintf(RESPONSE, "Eh?\r\n");
				os_sprintf(strValue, "Eh?");
			}

			if (isQuery) { // saves repeating this over and over in commands
				if (messageType == WASSERIAL)
					iprintf(RESPONSE, "%s=%s\r\n", tBuf, strValue);
				else
					MQTT_Publish(client, tBuf, strValue, os_strlen(strValue), 0, 0);
			}

		}     // loop through multiple messages before initiating a single possible save (minimise writes)
		if (doUpdate == 1) cfgSave();
	}
}

void IFA mqtt_setup()
{
	MQTT_InitConnection(&mqttClient, sysCfg.mqttHost, sysCfg.mqttPort, sysCfg.security);
	MQTT_InitClient(&mqttClient, sysCfg.deviceId, sysCfg.mqttUser, sysCfg.mqttPass, sysCfg.mqttKeepalive, 1);
	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);
}

void IFA mqtt_init()
{
	// Set up a timer to read the temperature
	os_timer_disarm(&temperatureTimer);
	os_timer_setfn(&temperatureTimer, (os_timer_func_t *) temperature_cb, (void *) 0);
	os_timer_arm(&temperatureTimer, TEMPERATURE_READING_DELAY, 1);

	os_timer_disarm(&rtcTimer);
	os_timer_setfn(&rtcTimer, (os_timer_func_t *) rtc_cb, (void *) 0);
	os_timer_arm(&rtcTimer, 1000, 1);

	// Set up a led fader timer 20 times a second
	os_timer_disarm(&ledTimer);
	os_timer_setfn(&ledTimer, (os_timer_func_t *) led_cb, (void *) 0);
	os_timer_arm(&ledTimer, 50, 1);

	// my silly LED clock
	os_timer_disarm(&clockTimer);
	os_timer_setfn(&clockTimer, (os_timer_func_t *) clock_cb, (void *) 0);
	//os_timer_arm(&clockTimer, 16, 1);
	os_timer_arm(&clockTimer, 1000, 1);

	// RGB timer
	os_timer_disarm(&rgbTimer);
	os_timer_setfn(&rgbTimer, (os_timer_func_t *) rgb_cb, (void *) 0);
	os_timer_arm(&rgbTimer, 250, 1);

	// off to get the time
	sntp_setservername(0, "us.pool.ntp.org");
	sntp_setservername(1, "ntp.sjtu.edu.cn");
	sntp_init();
	mqtt_setup();
}

void IFA petes_initialisation(void)
{
	uart_init(115200, 115200);
	cfgLoad(0);
	wifi_set_opmode(0);
	// system_set_os_print(1);
	secondsUp = 0;

	indic_buf[0] = 0;
	indic_buf[1] = 0;
	indic_buf[2] = 0;

	flashIn = 0; flashOut = 0; // flash queue
	setFlash(INDIC_POWER);

	if ((sysCfg.system_clock == 80) || (sysCfg.system_clock == 160))
	{ system_update_cpu_freq(sysCfg.system_clock); }
	buzzer = 0;
	if ((sysCfg.relayoutput_override != 16) && (sysCfg.relayoutput_override != 0))
		sysCfg.relayoutput_override = 16;

	// init 4 and 5 only if sysCfg.serial2 is zero

	easygpio_pinMode(sysCfg.relayoutput_override, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);

	easygpio_pinMode(12, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
	easygpio_pinMode(13, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
	easygpio_pinMode(14, EASYGPIO_PULLUP, EASYGPIO_INPUT);
	easygpio_pinMode(15, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
	if (sysCfg.enable16) easygpio_pinMode(16, EASYGPIO_NOPULL, EASYGPIO_OUTPUT); // new boards using pin 0 for input - enable 16 for relay etc...

	iprintf(INFO, "\r\n\r\n-----------------------------\r\nESP Starting...\r\n");

	switch (sysCfg.serial2)
	{
	case 0 : // normal use
		easygpio_pinMode(5, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
		easygpio_pinMode(4, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
		iprintf(INFO, "GPIO4 and 5 are outputs.\r\n");
		break;

	case 1 : //tristate
		easygpio_pinMode(5, EASYGPIO_NOPULL, EASYGPIO_INPUT);
		easygpio_pinMode(4, EASYGPIO_NOPULL, EASYGPIO_INPUT);
		iprintf(INFO, "GPIO4 and 5 are inputs.\r\n");
		break;

	case 2 :
	case 3 :
	case 4 :
	case 5 :
		Softuart_SetPinRx(&softuart2, 5);
		Softuart_SetPinTx(&softuart2, 4);
		iprintf(INFO, "Initialised software serial to ");
		switch (sysCfg.serial2)
		{
		case 2: Softuart_Init(&softuart2, 57600); iprintf(INFO, "57600"); break;
		case 3: Softuart_Init(&softuart2, 38400); iprintf(INFO, "38400"); break;
		case 4: Softuart_Init(&softuart2, 19200); iprintf(INFO, "19200"); break;
		case 5: Softuart_Init(&softuart2, 9600); iprintf(INFO, "9600"); break;
		}
		Softuart_Writeline_Nextion(&softuart2, " ");
		iprintf(INFO, " baud.\r\n");
		break;
	}

	analogAccum = system_adc_read() * 10;
	analog = analogAccum / 10;

	pwm_array.red = 0; pwm_array.reda = 0;
	pwm_array.green = 0; pwm_array.greena = 0;
	pwm_array.blue = 0; pwm_array.bluea = 0;
	rgbPlayBuffer[0] = -1; rgbPlayPtr = 0; rgbRecordPtr = 0;

	inBounceCount = 0;

	mist.repeats = 0; // special for the likes of garden misters.


	if (sysCfg.out14Enable == 1)
	{
		easygpio_pinMode(14, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
		(sysCfg.newInvert & 1) ? easygpio_outputSet(14, ((sysCfg.out14Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(14, ((sysCfg.out14Status == 1) ? OUT_ON : OUT_OFF));
	}

	if (sysCfg.set2Out == 1)
	{
		easygpio_pinMode(2, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
		(sysCfg.invert & 2) ? easygpio_outputSet(2, ((sysCfg.out2Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(2, ((sysCfg.out2Status == 1) ? OUT_ON : OUT_OFF));
	}
	else
		easygpio_pinMode(2, EASYGPIO_PULLUP, EASYGPIO_INPUT);

	// use 4 and 5 only if serial 2 is zero
	if ((sysCfg.serial2 == 0) && (sysCfg.sonoff == 0) ){
		(sysCfg.invert & 4) ? easygpio_outputSet(5, ((sysCfg.out5Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(5, ((sysCfg.out5Status == 1) ? OUT_ON : OUT_OFF));
		(sysCfg.invert & 8) ? easygpio_outputSet(4, ((sysCfg.out4Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(4, ((sysCfg.out4Status == 1) ? OUT_ON : OUT_OFF));
	}

	(sysCfg.invert & 1) ? easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.out0Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(sysCfg.relayoutput_override, ((sysCfg.out0Status == 1) ? OUT_ON : OUT_OFF));


	if (sysCfg.enable16)
		{
		(sysCfg.invert & 128) ? easygpio_outputSet(16, ((sysCfg.out16Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(16, ((sysCfg.out16Status == 1) ? OUT_ON : OUT_OFF));
		}


	(sysCfg.invert & 64) ? easygpio_outputSet(15, ((sysCfg.out15Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(15, ((sysCfg.out15Status == 1) ? OUT_ON : OUT_OFF));
	(sysCfg.invert & 16) ? easygpio_outputSet(12, ((sysCfg.out12Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(12, ((sysCfg.out12Status == 1) ? OUT_ON : OUT_OFF));
	(sysCfg.invert & 32) ? easygpio_outputSet(13, ((sysCfg.out13Status == 1) ? OUT_OFF : OUT_ON)) :  easygpio_outputSet(13, ((sysCfg.out13Status == 1) ? OUT_ON : OUT_OFF));

	timeoutCount = sysCfg.timeoutPeriod; // set up this useful watchdog BEFORE setting up seconds callback as it is set there.
	if (sysCfg.timeoutPeriod) easygpio_outputSet(sysCfg.timeoutPin, (sysCfg.timeoutInvert) ? OUT_ON : OUT_OFF);

	tm.Valid = 0;     //ps time is invalid

	rgb.reda = 0;
	rgb.greena = 0;
	rgb.bluea = 0;
	rgb.red = 0;
	rgb.green = 0;
	rgb.blue = 0;
	rgb.rainbow = 0;

	int a; for (a=0;a<8;a++) newports[a]=255; // all high to start with i2c extender ports (xport)

	pinChangeDownCounter = 0;

	// Set up a powerup timer for real time clock 50ms
	os_timer_disarm(&startupTimer);
	os_timer_setfn(&startupTimer, (os_timer_func_t *) startup_cb, (void *) 0);
	os_timer_arm(&startupTimer, 200, 1);

	// Set up a flash indicator timer
	os_timer_disarm(&flashTimer);
	os_timer_setfn(&flashTimer, (os_timer_func_t *) flash_cb, (void *) 0);
	os_timer_arm(&flashTimer, 25, 1);

	// Set up a debounce timer 20 times a second
	os_timer_disarm(&bounceTimer);
	os_timer_setfn(&bounceTimer, (os_timer_func_t *) bounce_cb, (void *) 0);
	os_timer_arm(&bounceTimer, 25, 1);


	// Set up a timer for serial messages don't start it - still can't fire < 80ms to mqtt_cb
	os_timer_disarm(&serialTimer);
	os_timer_setfn(&serialTimer, (os_timer_func_t *) serial_cb, (void *) 0);
	os_timer_arm(&serialTimer, 15, 1);

	// Set up a timer with 15 second interval for checking WIFI...
	os_timer_disarm(&lostThePlotTimer);
	os_timer_setfn(&lostThePlotTimer, (os_timer_func_t *) lostThePlot_cb, (void *) 0);
	os_timer_arm(&lostThePlotTimer, 4000, 1); // every 4 secs for WIFI
	timeTimeout = 0;

	iprintf(INFO, "Current web programming pin: %d\r\n", sysCfg.wifi_button);
	if ((sysCfg.electrodragon) && (sysCfg.rgb_ind == 0)) iprintf(INFO, "GPIO16");
	else iprintf(INFO, "GPIO13 ");
	if (sysCfg.rgb_ind == 0)
		iprintf(INFO, "is a LED indicator.\r\n");
	else
		iprintf(INFO, "is a serial RGB LED indicator.\r\n");
	iprintf(INFO, "Software version %s\r\n", SYSTEM_VER);
	iprintf(RESPONSE, "SDK Version: %s\r\n", system_get_sdk_version());
	iprintf(INFO, "Use the {debug} command for more information.\r\n");

	if (sysCfg.wifiSetup == 1)
	{
		setFlash(INDIC_WEB); // web setup mode
	}
	else
	{
		setFlash(INDIC_START); // normal mode
	}

	if ((sysCfg.rgbPermPort >= 0) && (sysCfg.rgbPermPort <= 16))
	{
		rgb.red = sysCfg.rgbPermRed;
		rgb.green = sysCfg.rgbPermGreen;
		rgb.blue = sysCfg.rgbPermBlue;
		rgb.rgbnum = sysCfg.rgbPermNum;
		rgb.rgbdelay = 4;
		os_delay_us(100000);
		easygpio_outputSet(sysCfg.rgbPermPort,0);
		os_delay_us(100000);
		rgblights(sysCfg.rgbPermPort, rgb.red, rgb.green, rgb.blue, rgb.rgbnum); // restore
		os_delay_us(100000);
		rgblights(sysCfg.rgbPermPort, rgb.red, rgb.green, rgb.blue, rgb.rgbnum); // restore to be sure
	}

	if (sysCfg.iliGo==45) iliStartup();

}
