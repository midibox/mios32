// $Id: app.c 690 2009-08-04 22:49:33Z tk $
/*
 * MIOS32 Application Template
 *
 * ==========================================================================
 *
 *  Copyright (C) <year> <your name> (<your email address>)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <conio.h>
#include <kbd.h>
#include <ui.h>
#include "app.h"
#include "aa.c" //ascii art

extern void MicroVGA_Init(void); 


ROMDEF static char *memview_fkeys[10] = 
{"Help",0,0,0,"DynMem",0,0,0,0,"Quit"};

ROMDEF static char *mainmenu[] = 
{ 
  "Memory viewer",
  "Console debug demo",
  "Speed test",
  "Test sub menu",
  "Command line interface",
  "ASCII-Art test",
  "Input dialog test",
  0
};

ROMDEF static char *submenu1[] = 
//far const rom char *submenu1[] = 
{ 
  "Menu item number 1",
  "Menu item number 2",
  "Some other menu",
  "And finally last menu item",
  0
};

const static char hexchars[16] = "0123456789ABCDEF";

void drawmem(const unsigned char *mem)
{
  const unsigned char *mptr;
  int i,j;


 textbackground(BLUE);
 for (i=0;i<23 && !_kbhit();i++) {
   mptr = &mem[i*16];
   gotoxy(1,i+2);
   textcolor(YELLOW);
   j = 8;
   do {
     j--;
     _putch(hexchars[  (((int)mptr)>>(j*4)) &0xF] );
   } while (j>0);
   _cputs(":  ");
   textcolor(WHITE);

   for (j=0;j<16;j++) {
     _putch(hexchars[*mptr>>4]);
     _putch(hexchars[*mptr&0xF]);
     mptr++;
     _putch(' ');
     if (j == 7) {
       _putch('-');
       _putch(' ');
     }
   } 

   _putch(179);
   _putch(' ');

   mptr = &mem[i*16];
   for (j=0;j<16;j++) {
     if (*mptr >= ' ')
     _putch(*mptr);
      else _putch('.');
     mptr++;
   } 

   clreol();   
 }
}


void memviewer(void *start)
{
 int key;
 const unsigned char *mem;

 textattr(LIGHTGRAY  | BLUE <<4);
 clrscr();
 textcolor(BLACK);
 textbackground(CYAN);
 _cputs("Memory viewer");
 clreol();

 cursoroff();
 drawfkeys(memview_fkeys);

 mem=(const unsigned char *)start;
 for (;;) {
   drawmem(mem);

//   if (_kbhit()) {
     key = _getch();
     switch(key) {
      case KB_LEFT: if ((int)mem > 0 ) (int)mem--; break;
      case KB_RIGHT: mem++; break;
      case KB_UP: if (mem >= (unsigned char const *)16) mem-=16; break;
      case KB_DOWN: mem+=16; break;
      case KB_PGUP: if (mem >= (unsigned char const *)(16*23))  mem-=16*23; break;
      case KB_PGDN: mem+=16*23; break;
      case KB_F10: cursoron(); return ;
     } 
  // }

 }
}

void debugdemo()
{
 int key;

 textattr(LIGHTGRAY);
 clrscr();

 while (1) {
    _cputs("Debug line 1\r\n");
   if (_kbhit()) {
      key = _getch();
      _cputs("You have pressed key");
	  _putch(key);
      _cputs("\r\n");
      if (key == KB_ESC)
       return ;
   }
 }
}


void cli()
{
    char buffer[40];
    char * out;

    textattr(LIGHTGRAY);
    
    clrscr();
    
    while (1) {
      //read data
      buffer[0]=40;
       
      textattr(YELLOW);
      _cputs("uVGA> ");
      textattr(LIGHTGRAY);
      out=_cgets(buffer);
      
      if (out != 0 && out[0] == 'q' && out[1] == 0)
        return ;
      
      textattr(WHITE);
      _cputs("\r\nUnsupported command. Only ");
      textattr(LIGHTRED);
      _cputs("q");
      textattr(WHITE);
      _cputs(" (exit) is supported.\r\n");
    }
}


void speedtest(void)
{
  unsigned int i;

  gotoxy(1,1);
  textattr(GREEN<<4 | YELLOW);

  for (i=0;i<50000;i++) {
    _cputs("TEST50=>");
    _putch(((i/10000)%10) + '0');
    _putch(((i/1000)%10) + '0');
	_cputs("\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08");
  }
}


void submenutest()
{
int item;

 item = 0;

 while (1) {

	item = runmenu(8,10, submenu1, item);
	switch (item) {
		case 0: return ;
	}
 }
}


void ascii_art()
{
	 textattr(LIGHTGRAY);
	 clrscr();
	_cputs((const char *)ascii_art_str);
	
	 while (_getch() != KB_ESC);
}

void testCGetS()
{
        char buffer[40];
        char * out;
        int i;
             
        //draw input dialog (frame)
        drawframe(10, 10, 40, 9, WHITE  | RED <<4);
        gotoxy(12,12);
        _cputs("Enter your name:");       
        gotoxy(12,14);
        textattr(WHITE  | BLACK <<4);
        for (i=0;i<40;i++)
          _putch(' ');
        
        //read data
        gotoxy(12,14);
        buffer[0]=40;
         
        out=_cgets(buffer);
        
        //write read data
        gotoxy(12,16);
        if (buffer[1]==0)
        {
             textattr((WHITE | RED <<4)|BLINK);
            _cputs("You didn't enter your name!");
        }
        else
        {
             textattr((WHITE | RED <<4));
            _cputs("Your name is:\n");
            gotoxy(12,17);
            _cputs(out);
        }
        cursoroff();

        gotoxy(23,19);
        textattr((WHITE | RED <<4)|BLINK);
        _cputs("PRESS ESC TO EXIT");
        
        //wait for ESC press
        while (_getch() != KB_ESC);
}

static ROMDEF char *uvga_logo=
"        ##    ##   ####     #   "
"        ##    ##  ##  ##   ###  "
"        ##    ## ##    #  ## ## "
" ##  ## ##    ## ##      ##   ##"
" ##  ## ##    ## ##      ##   ##"
" ##  ## ##    ## ## #### #######"
" ##  ## ##    ## ##   ## ##   ##"
" ##  ##  ##  ##  ##   ## ##   ##"
" #####    ####    ##  ## ##   ##"
" ##        ##      ### # ##   ##"
" ##       MicroVGA for MIOS32   "
"##                              "
"www.ucapps.de   www.midibox.org ";


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);
  MicroVGA_Init(); 
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  int i,j;
  int item=0;
  MIOS32_MIDI_SendDebugMessage("App Background starting");
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
	
	 textbackground(BLUE);
	 clrscr();
	 textcolor(BLACK);
	 textbackground(CYAN);
	 _cputs("MIOS32 demo app");

	 clreol();
	 textbackground(BLUE);
	 textcolor(GREEN);


	 // draw "logo"
     for (i=0;i<13;i++) {
	    gotoxy(46,12+i);
	     for (j=0;j<32;j++) {
	        if (uvga_logo[i*32+ j] == '#')
	           _putch(219); 
	        else _putch(uvga_logo[i*32+ j]);
	     }
	 }


	item = runmenu(5,5, mainmenu, item);
	switch (item) {
	  case 1:  memviewer(0); break;
	  case 2:  debugdemo(); break;
	  case 3:  speedtest(); break;
	  case 4:  submenutest(); break;
	  case 5:  cli(); break;
	  case 6:  ascii_art(); break;
          case 7:  testCGetS(); break;
	}
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
