#include "espmissingincludes.h" // ncherry@linuxha.com

#include "../include/driver/i2c.h"
#include "aidan_and_petes.h"

//
// BMP280 - Data largely torn out of and modified Edwin Cho's library but gutted as I2c here is different
// https://developer.mbed.org/users/12104404/code/BMP280/file/c72b726c7dc9/BMP280.cpp
//

char address=0;
uint16_t    dig_T1;
int16_t     dig_T2, dig_T3;
uint16_t    dig_P1;
int16_t     dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
uint16_t    dig_H1, dig_H3;
int16_t     dig_H2, dig_H4, dig_H5, dig_H6;
int32_t     t_fine;



int ICACHE_FLASH_ATTR BMP280_initialize(char addr)
{
    //char cmd[18];
    uint8_t cmd[18];
    // int x;  // unused

    address=addr;
	i2c_master_gpio_init();

    cmd[0] = 0x01; // Humidity oversampling x1
    i2c_writeData(address, 0xf2, cmd, 1);

    cmd[0] = 0x27; // Temperature oversampling x1, Pressure oversampling x1, Normal mode
    i2c_writeData(address, 0xf4, cmd, 1);

    cmd[0] = 0xa0; // Standby 1000ms, Filter off
    i2c_writeData(address, 0xf5, cmd, 1);

    i2c_readData(address, 0x88, cmd, 6);

    dig_T1 = (cmd[1] << 8) | cmd[0];
    dig_T2 = (cmd[3] << 8) | cmd[2];
    dig_T3 = (cmd[5] << 8) | cmd[4];

    i2c_readData(address, 0x8e, cmd, 18); // read dig_P regs

    dig_P1 = (cmd[ 1] << 8) | cmd[ 0];
    dig_P2 = (cmd[ 3] << 8) | cmd[ 2];
    dig_P3 = (cmd[ 5] << 8) | cmd[ 4];
    dig_P4 = (cmd[ 7] << 8) | cmd[ 6];
    dig_P5 = (cmd[ 9] << 8) | cmd[ 8];
    dig_P6 = (cmd[11] << 8) | cmd[10];
    dig_P7 = (cmd[13] << 8) | cmd[12];
    dig_P8 = (cmd[15] << 8) | cmd[14];
    dig_P9 = (cmd[17] << 8) | cmd[16];

   return 0;
}




int16_t ICACHE_FLASH_ATTR BMP280_getTemperature()
{
    uint32_t temp_raw;
    //float tempf;
    //char cmd[3];
    uint8_t cmd[3];
    //int x;

    i2c_readData(address, 0xfa, cmd, 3); // temp_msb

    temp_raw = (cmd[0] << 12) | (cmd[1] << 4) | (cmd[2] >> 4);

    int32_t temp;

    temp =
        (((((temp_raw >> 3) - (dig_T1 << 1))) * dig_T2) >> 11) +
        ((((((temp_raw >> 4) - dig_T1) * ((temp_raw >> 4) - dig_T1)) >> 12) * dig_T3) >> 14);

    t_fine = temp;
    temp = (temp * 5 + 128) >> 8;
    //tempf = (float)temp;

    //return (tempf/100.0f);
    return (int16_t) (temp/10);
}

int16_t ICACHE_FLASH_ATTR  BMP280_getPressure()
{
    uint32_t press_raw;
   // float pressf;
    //char cmd[3];
    uint8_t cmd[3];
    //int x;

    i2c_readData(address, 0xf7, cmd, 3); // press_msb

    press_raw = (cmd[0] << 12) | (cmd[1] << 4) | (cmd[2] >> 4);

    int32_t var1, var2;
    uint32_t press;

    var1 = (t_fine >> 1) - 64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * dig_P6;
    var2 = var2 + ((var1 * dig_P5) << 1);
    var2 = (var2 >> 2) + (dig_P4 << 16);
    var1 = (((dig_P3 * (((var1 >> 2)*(var1 >> 2)) >> 13)) >> 3) + ((dig_P2 * var1) >> 1)) >> 18;
    var1 = ((32768 + var1) * dig_P1) >> 15;
    if (var1 == 0) {
        return 0;
    }
    press = (((1048576 - press_raw) - (var2 >> 12))) * 3125;
    if(press < 0x80000000) {
        press = (press << 1) / var1;
    } else {
        press = (press / var1) * 2;
    }
    var1 = ((int32_t)dig_P9 * ((int32_t)(((press >> 3) * (press >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(press >> 2)) * (int32_t)dig_P8) >> 13;
    press = (press + ((var1 + var2 + dig_P7) >> 4));

    //pressf = (float)press;
    //return (pressf/100.0f);
    return (int16_t) (press/10);
}



