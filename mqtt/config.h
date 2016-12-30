/* config.h
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

#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_
#include "os_type.h"
#include "user_config.h"
typedef struct{
    uint32_t cfg_holder;

    uint8_t stationSsid[32];
    uint8_t stationPwd[64];

    uint8_t stationSsid2[32];
    uint8_t stationPwd2[64];

    uint8_t stationSwap;

    uint32_t stationType;

    uint8_t deviceId[64];
    uint8_t mqttHost[64];
    uint32_t mqttPort;
    uint8_t mqttUser[32];
    uint8_t mqttPass[32];
    uint8_t description[64];
    uint8_t attribute[64]; // Unit attributes

    uint8_t webUser[32];
    uint8_t webPassword[32];
    uint16_t enableWebPageControl;

    uint32_t mqttKeepalive;

    uint16_t dawn;
    uint16_t dusk;
    uint8_t peak;
    uint8_t offPeak;
    uint8_t frost;
    uint16_t onOne;
    uint16_t offOne;
    uint16_t onTwo;
    uint16_t offTwo;
    uint8_t sensor_type;
    uint8_t out0Status;
    uint8_t out4Status;
    uint8_t out5Status;
    uint8_t out12Status;
    uint8_t out13Status;
    uint8_t in14Bounce;
    uint8_t enable13;
    uint8_t override0;
    uint32_t override0Time;
    uint8_t base[32];
    uint8_t invert;
    uint8_t security;
    uint16_t calibrate; // ADC Calibration
    uint8_t wifiSetup; // reboot and go into setup mode
    uint8_t mqttSubscribe[4][32]; // don't even think this is used!!!
    uint8_t tcpAddress[4];
    uint8_t tcpGateway[4];
    uint8_t tcpNetmask[4];
    uint16_t checksum;
    uint8_t clock;
    uint8_t out2Status;
    uint8_t out15Status;
    uint8_t lastFail;

    uint8_t otaHost[64];
    uint32_t otaPort;
    uint8_t serial2;
    uint8_t out16Status;
    uint16_t ssid_retries;

    uint8_t relayoutput_override;
    uint8_t system_clock;
    uint8_t rgb_ind;
    uint8_t wifi_button; // if 2 - use 0 for relay otherwise use 16 for relay...
    uint8_t sonoff;

    uint16_t timeoutPeriod;
    uint16_t timeoutSecs;
    uint8_t timeoutRepeat;
    uint8_t timeoutPin;
    uint8_t timeoutInvert;

    uint8_t electrodragon;
    uint8_t enable16;
    uint8_t iliGo;
    uint8_t spare4;
    uint8_t in2Bounce;
    uint8_t temperaturePort;
    uint8_t set2Out;
    int16_t rgbPermPort;
    uint8_t rgbPermRed;
    uint8_t rgbPermGreen;
    uint8_t rgbPermBlue;
    int16_t rgbPermNum;
    uint8_t OTAdone;
    uint8_t out14Enable;
    uint8_t out14Status;
    uint8_t newInvert;

    uint8_t spare_A;
    uint8_t spare_B;
    uint8_t spare_C;
    uint8_t spare_D;
    uint8_t spare_E;
    uint8_t spare_F;

    int16_t rgbPlayBuffer[RGBMAX];

} SYSCFG;

typedef struct {
    uint8 flag;
    uint8 pad[3];
} SAVE_FLAG;


void ICACHE_FLASH_ATTR cfgSave();
void ICACHE_FLASH_ATTR cfgLoad();

extern SYSCFG sysCfg;

#endif /* USER_CONFIG_H_ */
