#ifndef _SSD1306_H
#define _SSD1306_H
        
#define BLACK 0
#define WHITE 1
#define INVERSE 2   

#define line_length 21

typedef enum{
    SCROLL_RIGHT = 0x26,
    SCROLL_LEFT = 0x2A
}SCROLL_DIR;

typedef enum{
    SCROLL_SPEED_0 = 0x03,  // slowest
    SCROLL_SPEED_1 = 0x02,
    SCROLL_SPEED_2 = 0x01,
    SCROLL_SPEED_3 = 0x06,
    SCROLL_SPEED_4 = 0x00,
    SCROLL_SPEED_5 = 0x05,
    SCROLL_SPEED_6 = 0x04,
    SCROLL_SPEED_7 = 0x07   // fastest
}SCROLL_SPEED;

typedef enum{
    SCROLL_PAGE_0 = 0,
    SCROLL_PAGE_1,
    SCROLL_PAGE_2,
    SCROLL_PAGE_3,
    SCROLL_PAGE_4,
    SCROLL_PAGE_5,
    SCROLL_PAGE_6,
    SCROLL_PAGE_7   
}SCROLL_AREA;

#define DISPLAY_PAGE_MAX 3	//

char ScreenData[DISPLAY_PAGE_MAX][4][21];

//extern char ScreenData[3]];


void ICACHE_FLASH_ATTR display_redraw(void);
void display_next_page(void);
void display_prev_page(void);
void display_data(uint8 page, uint8 line, char data[20]);

//const uint8 oled_addr = 0x3D;
volatile uint8 display_page;

void display_init(void );
void display_off( void);
void display_on( void);
void display_update(void);
void display_clear(void);
void display_stopscroll(void);
void display_scroll( SCROLL_AREA start, SCROLL_AREA end, SCROLL_DIR dir, SCROLL_SPEED speed );
void display_contrast( uint8_t contrast );
void display_invert( uint8_t invert );

void ICACHE_FLASH_ATTR gfx_init( int16_t width, int16_t height );
void gfx_drawPixel(int16_t x, int16_t y, uint16_t color);
void gfx_drawLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color );
void gfx_setCursor( int16_t x, int16_t y );
void gfx_setTextSize( uint8_t size );
void gfx_setTextColor( uint16_t color );
void gfx_setTextBg( uint16_t background );
void gfx_write( uint8_t ch );
int16_t gfx_width(void);
int16_t gfx_height(void);
void gfx_print( const char* s );
void gfx_println( const char* s );
void gfx_drawRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color );
void gfx_fillRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color );
void gfx_drawCircle( int16_t x0, int16_t y0, int16_t r,uint16_t color );
void gfx_drawTriangle( int16_t x0, int16_t y0,int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color );
void gfx_setRotation( uint8_t x );

#endif	// _SSD1306_H

