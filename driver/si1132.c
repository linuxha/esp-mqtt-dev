/*
 * si1132.c
 *
 *  Created on: 3 Dec 2016
 *      Author: Peter
 */

#include "espmissingincludes.h"
#include "aidan_and_petes.h"
#include "si1132.h"
#include "../include/driver/i2c.h"
// shamelessly stolen and slightly modified from hardkernel driver - thank you
// https://github.com/hardkernel/WEATHER-BOARD/blob/master/libraries/ODROID_Si1132/ODROID_Si1132.cpp

// ncherry@linuxha.com
// more of this silliness
extern void os_delay_us(uint32_t t);
extern void i2c_general(uint8 device_addr, uint8* data, uint16_t slength, uint16_t rlength);
extern void IFA ODROID_Si1132_write8(uint8_t reg, uint8_t val);

void IFA ODROID_Si1132_reset()
{
	ODROID_Si1132_write8(Si1132_REG_MEASRATE0, 0);
	ODROID_Si1132_write8(Si1132_REG_MEASRATE1, 0);
	ODROID_Si1132_write8(Si1132_REG_IRQEN, 0);
	ODROID_Si1132_write8(Si1132_REG_IRQMODE1, 0);
	ODROID_Si1132_write8(Si1132_REG_IRQMODE2, 0);
	ODROID_Si1132_write8(Si1132_REG_INTCFG, 0);
	ODROID_Si1132_write8(Si1132_REG_IRQSTAT, 0xFF);

	ODROID_Si1132_write8(Si1132_REG_COMMAND, Si1132_RESET);
	os_delay_us(10000);
	ODROID_Si1132_write8(Si1132_REG_HWKEY, 0x17);

	os_delay_us(10000);
}

uint8_t IFA ODROID_Si1132_read8(uint8_t reg)
{
	uint8_t parm[1];
	parm[0]=reg;
	i2c_general(Si1132_ADDR,parm,1,1);
	return parm[0];
}

uint16_t IFA ODROID_Si1132_read16(uint8_t reg)
{
	uint16_t ret;
	uint8_t parm[2];
	parm[0]=reg;
	i2c_general(Si1132_ADDR,parm,1,2);
	ret=(parm[1]<<8)+parm[0];
	return ret;
}

void IFA ODROID_Si1132_write8(uint8_t reg, uint8_t val)
{
	uint8_t parm[4];
	parm[0]=reg; parm[1]=val;
	i2c_general(Si1132_ADDR,parm,2,0);
}

uint8_t IFA ODROID_Si1132_writeParam(uint8_t p, uint8_t v)
{
	ODROID_Si1132_write8(Si1132_REG_PARAMWR, v);
	ODROID_Si1132_write8(Si1132_REG_COMMAND, p | Si1132_PARAM_SET);
	return ODROID_Si1132_read8(Si1132_REG_PARAMRD);
}


uint8_t IFA ODROID_Si1132_begin(void)
{
    i2c_master_gpio_init();
	ODROID_Si1132_reset();
	// enable UVindex measurement coefficients!
	ODROID_Si1132_write8(Si1132_REG_UCOEF0, 0x7B);
	ODROID_Si1132_write8(Si1132_REG_UCOEF1, 0x6B);
	ODROID_Si1132_write8(Si1132_REG_UCOEF2, 0x01);
	ODROID_Si1132_write8(Si1132_REG_UCOEF3, 0x00);

	// enable UV sensor
	ODROID_Si1132_writeParam(Si1132_PARAM_CHLIST, Si1132_PARAM_CHLIST_ENUV |
		Si1132_PARAM_CHLIST_ENALSIR | Si1132_PARAM_CHLIST_ENALSVIS);
	ODROID_Si1132_write8(Si1132_REG_INTCFG, Si1132_REG_INTCFG_INTOE);
	ODROID_Si1132_write8(Si1132_REG_IRQEN, Si1132_REG_IRQEN_ALSEVERYSAMPLE);

	ODROID_Si1132_writeParam(Si1132_PARAM_ALSIRADCMUX, Si1132_PARAM_ADCMUX_SMALLIR);
  // fastest clocks, clock div 1
	ODROID_Si1132_writeParam(Si1132_PARAM_ALSIRADCGAIN, 0);
  // take 511 clocks to measure
	ODROID_Si1132_writeParam(Si1132_PARAM_ALSIRADCCOUNTER, Si1132_PARAM_ADCCOUNTER_511CLK);
  // in high range mode
	ODROID_Si1132_writeParam(Si1132_PARAM_ALSIRADCMISC, Si1132_PARAM_ALSIRADCMISC_RANGE);

  // fastest clocks
	ODROID_Si1132_writeParam(Si1132_PARAM_ALSVISADCGAIN, 0);
  // take 511 clocks to measure
	ODROID_Si1132_writeParam(Si1132_PARAM_ALSVISADCCOUNTER, Si1132_PARAM_ADCCOUNTER_511CLK);
  // in high range mode (not normal signal)
	ODROID_Si1132_writeParam(Si1132_PARAM_ALSVISADCMISC, Si1132_PARAM_ALSVISADCMISC_VISRANGE);

	ODROID_Si1132_write8(Si1132_REG_MEASRATE0, 0xFF);
	ODROID_Si1132_write8(Si1132_REG_COMMAND, Si1132_ALS_AUTO);
	return true;
}

uint16_t IFA ODROID_Si1132_readUV()
{
	os_delay_us(10000);
	return ODROID_Si1132_read16(0x2c);
}

int16_t IFA ODROID_Si1132_readIR()
{
	os_delay_us(10000);
	return ((int16_t)((ODROID_Si1132_read16(0x24) - 250)/2.44)*14.5);
}

int16_t IFA ODROID_Si1132_readVisible()
{
	os_delay_us(10000);
	return ((int16_t)((ODROID_Si1132_read16(0x22) - 256)/0.282)*14.5);
}

