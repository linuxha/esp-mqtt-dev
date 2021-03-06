/*
Connector to let httpd use the espfs filesystem to serve the files in it.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "espmissingincludes.h"
#include <string.h>
#include <osapi.h>
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "debug.h"
#include "httpd.h"
#include "espfs.h"
#include "httpdespfs.h"

#include "webpage.h"


//This is a catch-all cgi function. It takes the url passed to it, looks up the corresponding
//path in the filesystem and if it exists, passes the file through. This simulates what a normal
//webserver would do with static files.

#include "config.h"
extern SYSCFG sysCfg;

static uint8_t html_buf[1010];
int next_web_logon_posn = 0;

#define HTML_HEAD

//static const uint8_t ICACHE_RODATA_ATTR  web_page[] = "****** Goodbye ******";


int ICACHE_FLASH_ATTR cgiEspFsHook(HttpdConnData *connData)
{
	EspFsFile *file=connData->cgiData;
	int len;
	char buff[1024];


	if (connData->conn==NULL)
		{
		////Connection aborted. Clean up.
		espFsClose(file);
		next_web_logon_posn = 0;
		return HTTPD_CGI_DONE;
		}

	if (file==NULL)
		{
		//First call to this cgi. Open the file so we can read it.
	iprintf(DEBUG,"====OPENING THE FILE 1 ====\r\n");

		file=espFsOpen(connData->url);
		if (file==NULL)
			{
			return HTTPD_CGI_NOTFOUND;
			}
		connData->cgiData=file;
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
		httpdEndHeaders(connData);
		return HTTPD_CGI_MORE;
	}

	len=espFsRead(file, buff, 1024);
	if (len>0) espconn_sent(connData->conn, (uint8 *)buff, len);
	if (len!=1024) {
		//We're done.
		espFsClose(file);
		return HTTPD_CGI_DONE;
	} else {
		//Ok, till next time.
		return HTTPD_CGI_MORE;
	}
}


//cgiEspFsTemplate can be used as a template.

typedef struct {
	EspFsFile *file;
	void *tplArg;
	char token[64];
	int tokenPos;
} TplData;

typedef void (* TplCallback)(HttpdConnData *connData, char *token, void **arg);




//================================================================================================

int ICACHE_FLASH_ATTR cgiEspFsTemplate(HttpdConnData *connData) {
	TplData *tpd=connData->cgiData;
	int len;
	int x, sp=0;
	char *e=NULL;
	char buff[1025];

	char *p;


	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		((TplCallback)(connData->cgiArg))(connData, NULL, &tpd->tplArg);
		espFsClose(tpd->file);
		os_free(tpd);
		return HTTPD_CGI_DONE;
	}

	if (tpd==NULL) {
		//First call to this cgi. Open the file so we can read it.
		tpd=(TplData *)os_malloc(sizeof(TplData));
		iprintf(DEBUG,"====OPENING THE FILE 2 ====\r\n");
		tpd->file=espFsOpen(connData->url);
		tpd->tplArg=NULL;
		tpd->tokenPos=-1;
		if (tpd->file==NULL) {
			return HTTPD_CGI_NOTFOUND;
		}
		connData->cgiData=tpd;
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
		httpdEndHeaders(connData);
		return HTTPD_CGI_MORE;
	}



	len=espFsRead(tpd->file, buff, 1024);

	if (len>0)
		{
		sp=0;
		e=buff;
		for (x=0; x<len; x++) {
			if (tpd->tokenPos==-1) {
				//Inside ordinary text.
				if (buff[x]=='%') {
					//Send raw data up to now
					if (sp!=0) httpdSend(connData, e, sp);
					sp=0;
					//Go collect token chars.
					tpd->tokenPos=0;
				} else {
					sp++;
				}
			} else {
				if (buff[x]=='%') {
					if (tpd->tokenPos==0) {
						//This is the second % of a %% escape string.
						//Send a single % and resume with the normal program flow.
						httpdSend(connData, "%", 1);
					} else {
						//This is an actual token.
						tpd->token[tpd->tokenPos++]=0; //zero-terminate token
						((TplCallback)(connData->cgiArg))(connData, tpd->token, &tpd->tplArg);

					}
					//Go collect normal chars again.
					e=&buff[x+1];
					tpd->tokenPos=-1;
				} else {
					if (tpd->tokenPos<(sizeof(tpd->token)-1)) tpd->token[tpd->tokenPos++]=buff[x];
				}
			}
		}
	}
	//Send remaining bit.
	if (sp!=0) httpdSend(connData, e, sp);
	if (len!=1024) {
		//We're done.

//		os_strncpy(html_buf, web_page, os_strlen(web_page));

/*		{
		int n = 0, i = 0;
		for (;;)
			{
			n = 0;
			while ((html_buf[n] = read_rom_uint8(web_page + n + (i * 1000))) && (n < 1000))
				++n;

			html_buf[n] = 0; // Terminate the copy
			httpdSend(connData, html_buf, os_strlen(html_buf));

			if (n < 1000) // Finished
				break;
			++i; // Inc' 'number of buffers sent' count
			}
		}
*/

		((TplCallback)(connData->cgiArg))(connData, NULL, &tpd->tplArg);
		espFsClose(tpd->file);
		return HTTPD_CGI_DONE;
	} else {
		//Ok, till next time.
		return HTTPD_CGI_MORE;
	}
}




