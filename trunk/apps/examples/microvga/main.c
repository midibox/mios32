/*
    main.c
    Main demo file. Using conio functions to display demo userinterface.
    Part of MicroVGA CONIO library / demo project
    Copyright (c) 2008-9 SECONS s.r.o., http://www.MicroVGA.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "h/conio.h"
#include "h/kbd.h"
#include "h/ui.h"

#ifdef AVR_MCU  //WinAvr - AVR MCU - has to be defined in project settings
#define LOWMEM 1
#endif

#ifdef __ICC78K__
#define LOWMEM 1
#endif

#if RENESAS_C //Renessa High performance Emb. Workshop- has to be defined in project settings
#define LOWMEM 1
#endif

#ifndef LOWMEM
#include "aa.c" //ascii art
#endif

extern void MCU_Init(void);

/*const static char *nc_fkeys[10] = 
{"Help","Menu","View","Edit","Copy","RenMov","Mkdir","Delete","Setup","Quit"};
*/

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
#ifndef LOWMEM
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
 #else
  textattr(LIGHTGRAY);
  clrscr();
  _cputs("Not enought memory for this feature");
	
   while (_getch() != KB_ESC);
 #endif
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
 #ifndef LOWMEM
	 textattr(LIGHTGRAY);
	 clrscr();
	_cputs((const char *)ascii_art_str);
	
	 while (_getch() != KB_ESC);
 #else
         textattr(LIGHTGRAY);
	 clrscr();
	_cputs("Not enought memory for this feature");
	
	 while (_getch() != KB_ESC);
 #endif
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

#ifndef LOWMEM
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
" ##    MicroVGA demo application"
"##                              "
"www.MicroVGA.com, www.secons.com";
#endif

void main()
{
 int i,j;
 int item;

//////////// Main routine

 MCU_Init();


 item = 0;

 while (1) {
	
	 textbackground(BLUE);
	 clrscr();
	 textcolor(BLACK);
	 textbackground(CYAN);
	 _cputs("MicroVGA demo app");

	 clreol();
	 textbackground(BLUE);
	 textcolor(GREEN);


#ifndef LOWMEM
	 // draw "logo"
     for (i=0;i<13;i++) {
	    gotoxy(46,12+i);
	     for (j=0;j<32;j++) {
	        if (uvga_logo[i*32+ j] == '#')
	           _putch(219); 
	        else _putch(uvga_logo[i*32+ j]);
	     }
	 }
#endif


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
