/*********************************************************************************
Copyright (c) 2016, Cosmin Plasoianu and bits from Peter Scargill
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 ********************************************************************************/

#ifndef DRIVER_I2C_H_
#define DRIVER_I2C_H_

#include "driver/i2c_master.h"
#include "c_types.h"

uint8_t ICACHE_FLASH_ATTR i2c_writeData(uint8 device_addr,
									uint8 register_addr,
									uint8* data,
									uint16_t length);
uint8_t ICACHE_FLASH_ATTR i2c_writeSameData(uint8 device_addr,
									uint8 register_addr,
									uint8* data,
									uint16_t length);
uint8_t ICACHE_FLASH_ATTR i2c_readData(uint8 device_addr,
									uint16 register_addr,
									uint8* data,
									uint16_t length);
uint8_t ICACHE_FLASH_ATTR i2c_readDataWithData(uint8 device_addr,
									uint8 register_addr,
									uint8 param,
									uint8* data,
									uint16_t length);
#endif /* DRIVER_I2C_H_ */
