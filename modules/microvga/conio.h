/*
    conio.h
    Standard conio routines.
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

#ifndef _INC_CONIO
#define _INC_CONIO

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__COLORS)
#define __COLORS
// Compatible with DOS/WIN CONIO.H
enum COLORS {
    BLACK = 0,          /* dark colors */
    RED, 
    GREEN,
    BROWN,
    BLUE,
    MAGENTA,
    CYAN,
    LIGHTGRAY,
    DARKGRAY,       /* light colors */
    LIGHTRED,
    LIGHTGREEN,
    YELLOW,
    LIGHTBLUE,
    LIGHTMAGENTA,
    LIGHTCYAN,
    WHITE
};

#define BLINK 128
#endif

//defines max coordinates for checking overflow
#define MAX_X 80
#define MAX_Y 25

// Compatible with Unix Curses
#define ACS_ULCORNER	(0xDA)	/* upper left corner */
#define ACS_LLCORNER	(0xC0)	/* lower left corner */
#define ACS_URCORNER	(0xBF)	/* upper right corner */
#define ACS_LRCORNER	(0xD9)	/* lower right corner */
#define ACS_HLINE		(0xC4)	/* horizontal line */
#define ACS_VLINE		(0xB3)	/* vertical line */
#define ACS_LTEE	(acs_map['t'])	/* tee pointing right */
#define ACS_RTEE	(acs_map['u'])	/* tee pointing left */
#define ACS_BTEE	(acs_map['v'])	/* tee pointing up */
#define ACS_TTEE	(acs_map['w'])	/* tee pointing down */
#define ACS_PLUS	(acs_map['n'])	/* large plus or crossover */
#define ACS_S1		(acs_map['o'])	/* scan line 1 */
#define ACS_S9		(acs_map['s'])	/* scan line 9 */
#define ACS_DIAMOND	(acs_map['`'])	/* diamond */
#define ACS_CKBOARD	(acs_map['a'])	/* checker board (stipple) */
#define ACS_DEGREE	(acs_map['f'])	/* degree symbol */
#define ACS_PLMINUS	(acs_map['g'])	/* plus/minus */
#define ACS_BULLET	(acs_map['~'])	/* bullet */
/* Teletype 5410v1 symbols begin here */
#define ACS_LARROW	(acs_map[','])	/* arrow pointing left */
#define ACS_RARROW	(acs_map['+'])	/* arrow pointing right */
#define ACS_DARROW	(acs_map['.'])	/* arrow pointing down */
#define ACS_UARROW	(acs_map['-'])	/* arrow pointing up */
#define ACS_BOARD	(acs_map['h'])	/* board of squares */
#define ACS_LANTERN	(acs_map['i'])	/* lantern symbol */
#define ACS_BLOCK	(acs_map['0'])	/* solid square block */


/*
 Low-level hardware routines
 Have to be defined by user
*/

extern void _putch (char ch);	/* Writes a character directly to the console. */
extern int _getch (void); 	/* Reads a character directly from the console, without echo. */
extern int _kbhit (void);       /* Determines if a keyboard key was pressed.*/

#ifdef PIC18
#define ROMDEF const rom
#else
#define ROMDEF const
#endif

/*
 Standard conio routines
 Hardware/architecture/compiler independent
*/
extern void _cputs(ROMDEF char *s);		/* Outputs a string directly to the console. */
extern char* _cgets(char *s);                   /* Gets a string directly from the console.  */

extern void clrscr(void);
extern void clreol(void);

extern void gotoxy(char x, char y);

extern void cursoron(void);
extern void cursoroff(void);


extern void textcolor(int color);
extern void textbackground(int color);
extern void textattr(int attr);


#ifdef  __cplusplus
}
#endif


#endif  /* _INC_CONIO */
