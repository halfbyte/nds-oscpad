/*---------------------------------------------------------------------------------
	$Id: template.c,v 1.4 2005/09/17 23:15:13 wntrmute Exp $

	Basic Hello World

	$Log: template.c,v $
	Revision 1.4  2005/09/17 23:15:13  wntrmute
	corrected iprintAt in templates
	
	Revision 1.3  2005/09/05 00:32:20  wntrmute
	removed references to IPC struct
	replaced with API functions
	
	Revision 1.2  2005/08/31 01:24:21  wntrmute
	updated for new stdio support

	Revision 1.1  2005/08/03 06:29:56  wntrmute
	added templates


---------------------------------------------------------------------------------*/
#include "nds.h"
#include <nds/arm9/console.h> //basic print funcionality
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <nds/arm9/ndsmotion.h>


#define ADDRESS "10.0.2.1"
#define PORT 7000

#define OUTPUT_BUFFER_SIZE 1024


#define		VCOUNT		(*((u16 volatile *) 0x04000006))

// wifi timer function, to update internals of sgIP
void Timer_50ms(void) {
   Wifi_Timer(50);
}

// notification function to send fifo message to arm7
void arm9_synctoarm7() { // send fifo message
   REG_IPC_FIFO_TX=0x87654321;
}

// interrupt handler to receive fifo messages from arm7
void arm9_fifo() { // check incoming fifo messages
   u32 value = REG_IPC_FIFO_RX;
   if(value == 0x87654321) Wifi_Sync();
}

char sendbuf[4096];
struct sockaddr_in sain;
int sock;
int portnum = PORT;


int copyOSCString(char *dest, char *src) {
  
  int len = 0;
  int paddedLen = 0;
  int words = 0;
  int i;

  strcpy(dest, src);
  len = strlen(src) + 1;
  
  if ((len % 4) > 0) { 
    words++;
    words = (len / 4) + 1;
    paddedLen = words * 4;
    for(i=len;i<paddedLen;i++)
      dest[i] = 0;
    len = paddedLen;
  }
  return len;
}

void sendOSCMessageI(char *url, long value) {
  int pos = 0;
  long convValue = 0;
  if(strlen(url) > 4095) return;
  pos += copyOSCString(&sendbuf[pos], url);
  pos += copyOSCString(&sendbuf[pos], ",i");
    
  convValue = ntohl(value);
  memcpy(&sendbuf[pos], &convValue, 4);
  pos += 4;
	sendto(sock,sendbuf,pos,0,(struct sockaddr *)&sain,sizeof(sain));	
}

void sendOSCMessageIII(char *url, long first, long second, long third) {
  int pos = 0;
  long convValue = 0;
  if(strlen(url) > 4095) return;
  pos += copyOSCString(&sendbuf[pos], url);
  pos += copyOSCString(&sendbuf[pos], ",iii");
  convValue = ntohl(first);
  memcpy(&sendbuf[pos], &convValue, 4);
  pos += 4;
  convValue = ntohl(second);
  memcpy(&sendbuf[pos], &convValue, 4);
  pos += 4;
  convValue = ntohl(third);
  memcpy(&sendbuf[pos], &convValue, 4);
  pos += 4;
	sendto(sock,sendbuf,pos,0,(struct sockaddr *)&sain,sizeof(sain));	
}

// yeah this looks pretty stupid.
void sendOSCMessageIIII(char *url, long first, long second, long third, long fourth) {
  int pos = 0;
  long convValue = 0;
  if(strlen(url) > 4095) return;
  pos += copyOSCString(&sendbuf[pos], url);
  pos += copyOSCString(&sendbuf[pos], ",iiii");
  convValue = ntohl(first);
  memcpy(&sendbuf[pos], &convValue, 4);
  pos += 4;
  convValue = ntohl(second);
  memcpy(&sendbuf[pos], &convValue, 4);
  pos += 4;
  convValue = ntohl(third);
  memcpy(&sendbuf[pos], &convValue, 4);
  pos += 4;
  convValue = ntohl(fourth);
  memcpy(&sendbuf[pos], &convValue, 4);
  pos += 4;
	sendto(sock,sendbuf,pos,0,(struct sockaddr *)&sain,sizeof(sain));	
}


void sendOSCMessageS(char *url, char *value) {
  int pos = 0;
  if(strlen(url) > 4095) return;
  pos += copyOSCString(&sendbuf[pos], url);
  pos += copyOSCString(&sendbuf[pos], ",s");
  pos += copyOSCString(&sendbuf[pos], value);    
	sendto(sock,sendbuf,pos,0,(struct sockaddr *)&sain,sizeof(sain));	
}
void sendOSCMessageSS(char *url, char *value1, char *value2) {
  int pos = 0;
  if(strlen(url) > 4095) return;
  pos += copyOSCString(&sendbuf[pos], url);
  pos += copyOSCString(&sendbuf[pos], ",ss");
  pos += copyOSCString(&sendbuf[pos], value1);
  pos += copyOSCString(&sendbuf[pos], value2);
	sendto(sock,sendbuf,pos,0,(struct sockaddr *)&sain,sizeof(sain));	
}


//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------

	//touchPosition touchXY;
  int i;
	unsigned long destip = 16908298;
	
  int ndsmotion;

	videoSetMode(0);	//not using the main screen
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	//sub bg 0 will be used to print text
	vramSetBankC(VRAM_C_SUB_BG);

	SUB_BG0_CR = BG_MAP_BASE(31);

	BG_PALETTE_SUB[255] = RGB15(31,31,31);	//by default font will be rendered with color 255

	//consoleInit() is a lot more flexible but this gets you up and running quick
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	iprintf("\n\n\tThis is OSCPad.\n");

  ndsmotion = motion_init();
  if(ndsmotion) {
    motion_set_offs_x();
    motion_set_offs_y();
    motion_set_offs_z();
    motion_set_offs_gyro();
    iprintf("NDSMotion calibrated\n");    
  } else {
    iprintf("NDSMotion not present\n");    
  }

	{ // send fifo message to initialize the arm7 wifi
		REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR; // enable & clear FIFO
		
		u32 Wifi_pass= Wifi_Init(WIFIINIT_OPTION_USELED);
   	REG_IPC_FIFO_TX=0x12345678;
   	REG_IPC_FIFO_TX=Wifi_pass;
   	
		*((volatile u16 *)0x0400010E) = 0; // disable timer3
		
		irqInit(); 
		irqSet(IRQ_TIMER3, Timer_50ms); // setup timer IRQ
		irqEnable(IRQ_TIMER3);
   	irqSet(IRQ_FIFO_NOT_EMPTY, arm9_fifo); // setup fifo IRQ
   	irqEnable(IRQ_FIFO_NOT_EMPTY);
   	
   	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_RECV_IRQ; // enable FIFO IRQ
   	
   	Wifi_SetSyncHandler(arm9_synctoarm7); // tell wifi lib to use our handler to notify arm7

		// set timer3
		*((volatile u16 *)0x0400010C) = -6553; // 6553.1 * 256 cycles = ~50ms;
		*((volatile u16 *)0x0400010E) = 0x00C2; // enable, irq, 1/256 clock
		
		while(Wifi_CheckInit()==0) { // wait for arm7 to be initted successfully
			while(VCOUNT>192); // wait for vblank
			while(VCOUNT<192);
		}
		
	} // wifi init complete - wifi lib can now be used!

	iprintf("Connecting via WFC data\n");
	{ // simple WFC connect:
		int i;
		Wifi_AutoConnect(); // request connect
		while(1) {
			i=Wifi_AssocStatus(); // check status
			if(i==ASSOCSTATUS_ASSOCIATED) {
				iprintf("Connected successfully!\n");
				break;
			}
			if(i==ASSOCSTATUS_CANNOTCONNECT) {
				iprintf("Could not connect!\n");
                while(1);
				break;
			}
		}
	} // if connected, you can now use the berkley sockets interface to connect to the internet!

    
   //shutdown(my_socket,0); // good practice to shutdown the socket.

    //closesocket(my_socket); // remove the socket.

	sock=socket(AF_INET,SOCK_DGRAM,0);
	sain.sin_family=AF_INET;
	sain.sin_port=htons(portnum);
	sain.sin_addr.s_addr=INADDR_ANY;
	bind(sock,(struct sockaddr *) &sain,sizeof(sain));
	ioctl(sock,FIONBIO,&i);	
	
	sain.sin_family=AF_INET;
	sain.sin_port=htons(portnum);
	sain.sin_addr.s_addr=destip;


    while(1) {
		while(VCOUNT>192);
		while(VCOUNT<192);
		scanKeys();
		
		////////////////// TOUCHSCREEN
		if((keysHeld()&KEY_TOUCH)) {
		  touchPosition touchXY;
		  touchXY=touchReadXY();
      iprintf("\x1b[10;0Htouch 1, touch 2 : %d, %d\n", touchXY.z1, touchXY.z2);
		  int intPressure = (touchXY.x * touchXY.z2) / (64 * touchXY.z1) - touchXY.x / 64;
			sendOSCMessageIII("/nds/touch", touchXY.px, touchXY.py, intPressure);
		}
		if(keysUp()&KEY_TOUCH) {
			sendOSCMessageS("/nds/touch", "UP");
		}
		if (keysDown()&KEY_TOUCH) {
		  sendOSCMessageS("/nds/touch", "DOWN");
		}

    ////////////////////// BUTTONS
		if(keysUp()&KEY_START) {
			sendOSCMessageSS("/nds/button", "START", "UP");
		}
		if(keysDown()&KEY_START) {
			sendOSCMessageSS("/nds/button", "START", "DOWN");
		}

		if(keysUp()&KEY_A) {
			sendOSCMessageSS("/nds/button", "A", "UP");
		}
		if(keysDown()&KEY_A) {
			sendOSCMessageSS("/nds/button", "A", "DOWN");
		}

		if(keysUp()&KEY_B) {
			sendOSCMessageSS("/nds/button", "B", "UP");
		}
		if(keysDown()&KEY_B) {
			sendOSCMessageSS("/nds/button", "B", "DOWN");
		}

		if(keysUp()&KEY_L) {
			sendOSCMessageSS("/nds/button", "L", "UP");
		}
		if(keysDown()&KEY_L) {
			sendOSCMessageSS("/nds/button", "L", "DOWN");
		}

		if(keysUp()&KEY_R) {
			sendOSCMessageSS("/nds/button", "R", "UP");
		}
		if(keysDown()&KEY_R) {
			sendOSCMessageSS("/nds/button", "R", "DOWN");
		}

		if(keysUp()&KEY_X) {
			sendOSCMessageSS("/nds/button", "X", "UP");
		}
		if(keysDown()&KEY_X) {
			sendOSCMessageSS("/nds/button", "X", "DOWN");
		}

		if(keysUp()&KEY_Y) {
			sendOSCMessageSS("/nds/button", "Y", "UP");
		}
		if(keysDown()&KEY_Y) {
			sendOSCMessageSS("/nds/button", "Y", "DOWN");
		}

		if(keysUp()&KEY_UP) {
			sendOSCMessageSS("/nds/button", "UP", "UP");
		}
		if(keysDown()&KEY_UP) {
			sendOSCMessageSS("/nds/button", "UP", "DOWN");
		}

		if(keysUp()&KEY_DOWN) {
			sendOSCMessageSS("/nds/button", "DOWN", "UP");
		}
		if(keysDown()&KEY_DOWN) {
			sendOSCMessageSS("/nds/button", "DOWN", "DOWN");
		}

		if(keysUp()&KEY_LEFT) {
			sendOSCMessageSS("/nds/button", "LEFT", "UP");
		}
		if(keysDown()&KEY_LEFT) {
			sendOSCMessageSS("/nds/button", "LEFT", "DOWN");
		}

		if(keysUp()&KEY_RIGHT) {
			sendOSCMessageSS("/nds/button", "RIGHT", "UP");
		}
		if(keysDown()&KEY_RIGHT) {
			sendOSCMessageSS("/nds/button", "RIGHT", "DOWN");
		}
    // just for the heck of it
		if(keysUp()&KEY_LID) {
			sendOSCMessageSS("/nds/button", "LID", "OPEN");
		}
		if(keysDown()&KEY_LID) {
			sendOSCMessageSS("/nds/button", "LID", "CLOSE");
		}

		/////////////////////// NDSMOTION (if present)
		if (ndsmotion) {
      int x=0;
      int y=0;
      int z=0;
      int g=0;
      x = motion_read_x();
      y = motion_read_y();
      z = motion_read_z();
      g = motion_read_gyro();
      sendOSCMessageIIII("/nds/motion", (long)x, (long)y, (long)z, (long)g);
		}
	}

	return 0;
}
