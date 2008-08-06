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
void sendOSCMessage() {
	int sock, i;
	int portnum = PORT;
	unsigned long destip = 16908298;
	struct sockaddr_in sain;
	strcpy(sendbuf,"/foo\0\0\0\0,i\0\0\0\0\0\1");
	
	sock=socket(AF_INET,SOCK_DGRAM,0);
	sain.sin_family=AF_INET;
	sain.sin_port=htons(portnum);
	sain.sin_addr.s_addr=INADDR_ANY;
	bind(sock,(struct sockaddr *) &sain,sizeof(sain));
	ioctl(sock,FIONBIO,&i);
	
	
	sain.sin_family=AF_INET;
	sain.sin_port=htons(portnum);
	sain.sin_addr.s_addr=destip;
	sendto(sock,sendbuf,16,0,(struct sockaddr *)&sain,sizeof(sain));
	
	closesocket(sock);
}


//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------

	//touchPosition touchXY;

	videoSetMode(0);	//not using the main screen
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	//sub bg 0 will be used to print text
	vramSetBankC(VRAM_C_SUB_BG);

	SUB_BG0_CR = BG_MAP_BASE(31);

	BG_PALETTE_SUB[255] = RGB15(31,31,31);	//by default font will be rendered with color 255

	//consoleInit() is a lot more flexible but this gets you up and running quick
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	iprintf("\n\n\tHello World!\n");

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

    while(1) {
		while(VCOUNT>192);
		while(VCOUNT<192);
		scanKeys();
		if(keysDown()&KEY_A) {
			sendOSCMessage();
			iprintf("sending packet\n");
		}
	};

	return 0;
}
