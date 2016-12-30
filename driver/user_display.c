#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"

#include "debug.h"
#include "driver/ssd1306.h"

extern char ScreenData[3][4][21];

void ICACHE_FLASH_ATTR
display_draw_page(uint8 page)
{
//    page--;
//	INFO("\r\npage:%d %s\r\n",page, ScreenData[page].line2);
//	INFO("\r\ncím: %d P0.2: %s\r\n", &ScreenData, ScreenData[0].line2);
//	INFO("\r\n page:%d \r\n", page);
//	INFO("\r\n átvesz: %d ", &ScreenData[0][0][0]);
//	INFO("P0.1: %s\r\n",  ScreenData[page][0]);

	gfx_setCursor( 0, 0 );
//	gfx_print( ScreenData[page].line1);
	gfx_print( &ScreenData[page][0][0] );

	gfx_setCursor( 0,  15 );
	if ( page ==0 || page ==1)
		gfx_setTextSize( 2 );
//	gfx_print( ScreenData[page].line2);
	gfx_print( &ScreenData[page][1][0] );

	gfx_setCursor( 0, 35 );
	gfx_setTextSize( 1 );
//	gfx_print( ScreenData[page].line3);
	gfx_print( &ScreenData[page][2][0] );

	gfx_setCursor( 0, 50 );
//	gfx_print( ScreenData[page].line4);
	gfx_print( &ScreenData[page][3][0] );

	display_update();
}
void ICACHE_FLASH_ATTR
//display_redraw(char *pagedata)
display_redraw()
{
    display_clear();
	display_draw_page(display_page);
}
void ICACHE_FLASH_ATTR
//display_redraw(char *pagedata)
display_next_page()
{
	display_page++;
    if (display_page == DISPLAY_PAGE_MAX) display_page = 0;
    display_clear();
	display_draw_page(display_page);
}
