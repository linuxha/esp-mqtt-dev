/*
 * petes.h
 *
 *  Created on: 16 Mar 2015
 *      Author: Aidan
 */

#ifndef USER_PETES_H_
#define USER_PETES_H_

#include <pwm.h>
#include <driver/gpio16.h>

uint8_t serialInOffset=0;
uint8_t serialOutOffset=0;
uint8_t serialBCount=0;
char serialInBuf[256];
char serialTBuf[128];

typedef struct
	{
		uint8_t reda;
		uint8_t greena;
		uint8_t bluea;

		uint8_t red;
		uint8_t green;
		uint8_t blue;

		uint32_t rainbow;
		uint16_t rgbnum;
		uint16_t rgbdelay;
		uint8_t buffer[RGB_BUFFSIZE];
	} LEDS;
LEDS rgb;

uint32_t timeTimeout;
uint32_t pinChangeDownCounter;
uint16_t startupTimeout=0;

LOCAL os_timer_t temperatureTimer;
LOCAL os_timer_t rtcTimer;
LOCAL os_timer_t bounceTimer;
LOCAL os_timer_t ledTimer;
LOCAL os_timer_t startupTimer;
LOCAL os_timer_t clockTimer;
LOCAL os_timer_t rgbTimer;
LOCAL os_timer_t lostThePlotTimer;
LOCAL os_timer_t otaTimer;
LOCAL os_timer_t flashTimer;
LOCAL os_timer_t serialTimer;

int temperature, humidity, pressure;
int highTemperature;

uint16_t analog,analogAccum;
uint8_t inBounceCount;
uint8_t in2BounceCount;
uint8_t in14Value;
uint8_t in2Value;
uint32_t in14Count = 0;
uint32_t in2Count =0;
uint8_t state13 = 0;
unsigned long myRtc = 0;
uint16_t gotDsReading = 0; // bitfield as could be on any port

#endif /* USER_PETES_H_ */
