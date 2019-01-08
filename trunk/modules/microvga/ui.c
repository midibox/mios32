/*
    ui.c
    User interface functions.
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

#include "conio.h"
#include "ui.h"
#include "kbd.h"


int runmenu(char x, char y, ROMDEF char *menu[], int defaultitem)
{
  int key, i,j, itemno;
  int nitems, width;
  ROMDEF char *s;

  itemno = defaultitem-1;
  width = 20;

  width = 10;
  nitems = 0;
  while (menu[nitems] != 0) {
    for (j=0;menu[nitems][j];j++);
    if (j>width)
     width = j;
    nitems++;
  }
  width+=2;

  if (itemno < 0 || itemno > nitems)
    itemno = 0;

  while (1) {


  cursoroff();
  textattr(CYAN<<4 | BLACK);
  gotoxy(x,y);
  _putch(ACS_ULCORNER);
  for (i=0;i<width+2;i++) 
	_putch(ACS_HLINE);
  _putch(ACS_URCORNER);

  for (i = 0;i<nitems;i++) {
	  gotoxy(x,y+i+1);
	  _putch(ACS_VLINE);
	  _putch(' ');
      if (i == itemno)
        textattr(YELLOW);
      s = 0;
	  for (j=0;j<width;j++) {
        if (s && *s)
          _putch(*s++);
	    else _putch(' ');
        if (s == 0)
          s = (ROMDEF char *)menu[i];
      }
       textattr(CYAN<<4 | BLACK);
	  _putch(' ');
	  _putch(ACS_VLINE);
  }

  gotoxy(x,y+nitems+1);
  _putch(ACS_LLCORNER);
  for (i=0;i<width+2;i++) 
	_putch(ACS_HLINE);
  _putch(ACS_LRCORNER);

  
  while (!_kbhit()) ;
  
   if (_kbhit()) {
     key = _getch();
     switch(key) {
      case KB_UP: if (itemno>0) itemno--;  else itemno = nitems-1; break;
      case KB_DOWN: itemno++; itemno %= nitems; break;
      case KB_ESC: cursoron(); return 0;
      case KB_ENTER: cursoron(); return itemno+1;
     } 
   } 
  }
}



void drawfkeys(ROMDEF char *fkeys[])
{
 ROMDEF char *s;
 int i, j;

 gotoxy(1,25);
 for (i=0;i<10;i++) {
   textcolor(WHITE);
   textbackground(BLACK);
   if (i!= 0)
    _putch(' ');
   if (i== 9) {
    _putch('1');
    _putch('0');
   } else
   _putch((i%10)+'1');
   textcolor(BLACK);
   textbackground(CYAN);

  s = fkeys[i] ? fkeys[i] : 0;
  for (j=0;j<6;j++) {
   if (s && *s)
    _putch(*s++);
   else _putch(' ');
  }
 }
}

void drawframe(int x, int y, int width, int height, int color)
{
  int i,j;
  
  textattr(color);
  gotoxy(x,y);
  
  _putch(ACS_ULCORNER);
  for (i=0;i<width+2;i++) 
	_putch(ACS_HLINE);
  _putch(ACS_URCORNER);
  
  for (i = 0;i<height;i++) {
	  gotoxy(x,y+i+1);
	  _putch(ACS_VLINE);
	  _putch(' ');
   
   for (j=0;j<width;j++) {
	    _putch(' ');
      }
          _putch(' ');
	  _putch(ACS_VLINE);
  }
 
  gotoxy(x,y+height+1);
  _putch(ACS_LLCORNER);
  for (i=0;i<width+2;i++) 
	_putch(ACS_HLINE);
  _putch(ACS_LRCORNER);
}