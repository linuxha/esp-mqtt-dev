/*
  rgb_lcd.cpp
  2013 Copyright (c) Seeed Technology Inc.  All right reserved.

  Author:Loovee
  2013-9-18

  add rgb backlight fucnction @ 2013-10-15
  
  The MIT License (MIT)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.1  USA

  Modified back to C code and changed to suit by P Scargill.
*/

#include <stdlib.h>
#include "espmissingincludes.h"
#include "osapi.h"

#include "driver/i2c.h"

#include "driver/rgb_lcd.h"

// ncherry@linuxha.com
// more of this silliness
extern void os_delay_us(uint32_t t);
extern void i2c_general(uint8 device_addr, uint8* data, uint16_t slength, uint16_t rlength);

void IFA rgb_lcd_begin(uint8_t cols, uint8_t lines, uint8_t dotsize)
{

	i2c_master_gpio_init();
    _displayfunction=0; //

    if (lines > 1) {
        _displayfunction |= LCD_2LINE;
    }
    _numlines = lines;
    _currline = 0;

    // for some 1 line displays you can select a 10 pixel high font
    if ((dotsize != 0) && (lines == 1)) {
        _displayfunction |= LCD_5x10DOTS;
    }
    else
        _displayfunction |= LCD_5x8DOTS;
    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
    os_delay_us(50000);


    // this is according to the hitachi HD44780 datasheet
    // page 45 figure 23

    // Send function set command sequence
    rgb_lcd_command(LCD_FUNCTIONSET | _displayfunction);
    os_delay_us(4500);  // wait more than 4.1ms

    // second try
    rgb_lcd_command(LCD_FUNCTIONSET | _displayfunction);
    os_delay_us(150);

    // third go
    rgb_lcd_command(LCD_FUNCTIONSET | _displayfunction);


    // finally, set # lines, font size, etc.
    rgb_lcd_command(LCD_FUNCTIONSET | _displayfunction);

    // turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    rgb_lcd_display();

    // clear it off
    rgb_lcd_clear();

    // Initialize to default text direction (for romance languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
    rgb_lcd_command(LCD_ENTRYMODESET | _displaymode);
    
    
    // backlight init
    rgb_lcd_setReg(REG_MODE1, 0);
    // set LEDs controllable by both PWM and GRPPWM registers
    rgb_lcd_setReg(REG_OUTPUT, 0xFF);
    // set MODE2 values
    // 0010 0000 -> 0x20  (DMBLNK to 1, ie blinky mode)
    rgb_lcd_setReg(REG_MODE2, 0x20);
    
    rgb_lcd_setRGB(255,255,255);

}

/********** high level commands, for the user! */
void IFA rgb_lcd_clear()
{
	rgb_lcd_command(LCD_CLEARDISPLAY);        // clear display, set cursor position to zero
    os_delay_us(2000);          // this command takes a long time!
}

void IFA rgb_lcd_home()
{
	rgb_lcd_command(LCD_RETURNHOME);        // set cursor position to zero
    os_delay_us(2000);        // this command takes a long time!
}

void IFA rgb_lcd_setCursor(uint8_t col, uint8_t row)
{

    col = (row == 0 ? col|0x80 : col|0xc0);
    unsigned char dta[2] = {0x80, col};

    i2c_general(LCD_ADDRESS,dta,2,0);

}

// Turn the display on/off (quickly)
void IFA rgb_lcd_noDisplay()
{
    _displaycontrol &= ~LCD_DISPLAYON;
    rgb_lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void IFA rgb_lcd_display() {
    _displaycontrol |= LCD_DISPLAYON;
    rgb_lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void IFA rgb_lcd_noCursor()
{
    _displaycontrol &= ~LCD_CURSORON;
    rgb_lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void IFA rgb_lcd_cursor() {
    _displaycontrol |= LCD_CURSORON;
    rgb_lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void IFA rgb_lcd_noBlink()
{
    _displaycontrol &= ~LCD_BLINKON;
    rgb_lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void IFA rgb_lcd_blink()
{
    _displaycontrol |= LCD_BLINKON;
    rgb_lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void IFA rgb_lcd_scrollDisplayLeft(void)
{
	rgb_lcd_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void IFA rgb_lcd_scrollDisplayRight(void)
{
	rgb_lcd_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void IFA rgb_lcd_leftToRight(void)
{
    _displaymode |= LCD_ENTRYLEFT;
    rgb_lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void IFA rgb_lcd_rightToLeft(void)
{
    _displaymode &= ~LCD_ENTRYLEFT;
    rgb_lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void IFA rgb_lcd_autoscroll(void)
{
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    rgb_lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void IFA rgb_lcd_noAutoscroll(void)
{
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    rgb_lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void IFA rgb_lcd_createChar(uint8_t location, uint8_t charmap[])
{
    int i;

    location &= 0x7; // we only have 8 locations 0-7
    rgb_lcd_command(LCD_SETCGRAMADDR | (location << 3));
    
    
    unsigned char dta[9];
    dta[0] = 0x40;
    for(i=0; i<8; i++)
    {
        dta[i+1] = charmap[i];
    }
    i2c_general(LCD_ADDRESS,dta,9,0);
}

// Control the backlight LED blinking
void IFA rgb_lcd_blinkLED(void)
{
    // blink period in seconds = (<reg 7> + 1) / 24
    // on/off ratio = <reg 6> / 256
	rgb_lcd_setReg(0x07, 0x17);  // blink every second
	rgb_lcd_setReg(0x06, 0x7f);  // half on, half off
}

void IFA rgb_lcd_noBlinkLED(void)
{
	rgb_lcd_setReg(0x07, 0x00);
	rgb_lcd_setReg(0x06, 0xff);
}

/*********** mid level commands, for sending data/cmds */

// send command
void IFA rgb_lcd_command(uint8_t value)
{
    unsigned char dta[2] = {0x80, value};
    i2c_general(LCD_ADDRESS,dta,2,0);
}

// send data
void IFA rgb_lcd_write(uint8_t value)
{

    unsigned char dta[2] = {0x40, value};
    i2c_general(LCD_ADDRESS,dta,2,0);
}

void IFA rgb_lcd_setReg(unsigned char addr, unsigned char dta)
{
 uint8_t x[2];
 x[0]=addr; x[1]=dta;
	i2c_general(RGB_ADDRESS,x,2,0);
}

void IFA rgb_lcd_setRGB(unsigned char r, unsigned char g, unsigned char b)
{
	rgb_lcd_setReg(REG_RED, r);
	rgb_lcd_setReg(REG_GREEN, g);
	rgb_lcd_setReg(REG_BLUE, b);
}
