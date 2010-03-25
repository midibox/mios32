#ifndef	_UWINDOWS_H_
#define	_UWINDOWS_H_

//#include "circle_api.h"
#include "list.h"
#include "UW_Config.h"



/************************************************************************************/
/* Micro Windows Types																*/	
/************************************************************************************/
typedef enum UW_Type
{
	UW_FORM			= 0,
	UW_BUTTON 		= 1,
	UW_LABEL		= 2,
    UW_EDIT        	= 3,
    UW_CHECKBOX		= 4,
    UW_ICON			= 5,
    UW_LISTBOX		= 6,
	UW_FADER		= 100,
	UW_POT			= 101,
	UW_ENVELOPE		= 102
} UW_Type;

typedef	struct 
{
	u32		u32TimeOut;
	u32		u32CurrentTime;
	void	(*Timer) (void * pWindowTimer);
} tstrTimer;

enum UW_ButtonState
    {
	PUSHED	 	=  0,		 /*!< Joystick was just pressed down.  */
    LEFT      	=  1,        /*!< Joystick was just pushed left.   */
    UP        	=  2,        /*!< Joystick was just pushed up.     */
    RIGHT     	=  3,        /*!< Joystick was just pushed right.  */
    DOWN      	=  4         /*!< Joystick was just pushed down.   */
};

typedef	struct
{
	void (*Click)(void * pWindowClicked);
	void (*ButtonPress) (void * pWindowClicked, enum UW_ButtonState state);
	tstrTimer	TimerCallBack;
} UW_CallBack;


typedef struct
{	
	struct		list_head	node;
	struct		list_head	items;
	UW_Type		type;
	u8			x,y;
	s32			Absx,Absy;
	s32			length,height;
	bool			visible,enable;
	u8	*		caption;
	u8			captionlen;
	s32			value;
	UW_CallBack	CallBacks;
	void *		ExtraProperties;
} UW_Window;

typedef struct
{
	struct		list_head	items;
	u8			ItemsCount;
	u8			ItemHeight;
	UW_Window*	ActiveItem;
	UW_Window*	StartItem;
	UW_Window	BtnUp,BtnDwn;
}UW_ListBox;

/************************************************************************************/
/* Default Object Sizes																	*/	
/************************************************************************************/

#define	DEFAULT_FORM_LENGTH		160
#define	DEFAULT_FORM_HEIGHT		104
#define	DEFAULT_BTN_LENGTH		16
#define	DEFAULT_BTN_HEIGHT		16
#define	DEFAULT_EDIT_LENGTH		16
#define	DEFAULT_EDIT_HEIGHT		16
#define	DEFAULT_CHECK_LENGTH	16
#define	DEFAULT_CHECK_HEIGHT	16
#define	DEFAULT_LABEL_LENGTH	16
#define	DEFAULT_LABEL_HEIGHT	16
#define	DEFAULT_LIST_LENGTH		16
#define	DEFAULT_LIST_HEIGHT		16
#define	DEFAULT_ICON_LENGTH		32
#define	DEFAULT_ICON_HEIGHT		32

// MIDIbox additions
#define	DEFAULT_FADER_LENGTH	4
#define	DEFAULT_FADER_HEIGHT	32
#define	DEFAULT_POT_LENGTH		8
#define	DEFAULT_POT_HEIGHT		8


/************************************************************************************/
/* Color and reduced Colors																*/	
/************************************************************************************/

/* CircleOS Definitions */

enum MENU_code
{
	MENU_LEAVE              = 0,                    
    MENU_CONTINUE           = 1,                    
    MENU_REFRESH            = 2,                    
    MENU_CHANGE             = 3,                    
    MENU_CONTINUE_COMMAND   = 4,                    
    MENU_LEAVE_AS_IT        = 5,                    
    MENU_RESTORE_COMMAND    = 6                     
};

#define RGB_MAKE(xR,xG,xB)    ( ( (xG&0x07)<<13 ) + ( (xG)>>5 )  +      \
                                 ( ((xB)>>3) << 8 )          +      \
                                 ( ((xR)>>3) << 3 ) )                    
#define RGB_RED         0x00F8                  
#define RGB_BLACK       0x0000                  
#define RGB_WHITE       0xffff                  
#define RGB_BLUE        0x1F00                  
#define RGB_GREEN       0xE007                  
#define RGB_YELLOW      (RGB_GREEN|RGB_RED)     
#define RGB_MAGENTA     (RGB_BLUE|RGB_RED)      
#define RGB_LIGHTBLUE   (RGB_BLUE|RGB_GREEN)    
#define RGB_ORANGE      (RGB_RED | 0xE001)      
#define RGB_PINK        (RGB_MAGENTA | 0xE001)  

#define 	GRAY             		(RGB_MAKE(180,180,180))
#define 	BLUE             		(RGB_BLUE)
#define		RED						(RGB_RED)
#define 	BLACK            		(RGB_BLACK)
#define 	WHITE            		(RGB_WHITE)
#define 	SIMPLE_BLACK		0x00
#define	SIMPLE_BLUE		0x01
#define	SIMPLE_GRAY		0x02
#define	SIMPLE_WHITE		0x03	
#define	CH_WIDTH			0x07
#define	CH_HEIGHT			0x10

#ifndef NULL
#define	NULL				0x00000000
#endif
/************************************************************************************/
/* Micro Windows Macros																*/	
/************************************************************************************/
#define	UW_SetType(pWindow,Type)	(pWindow->type = Type)
#define	UW_SetPosition(pWindow, x, y)	\
do										\
{										\
	(pWindow)->Absx 		= (s32)x;			\
	(pWindow)->Absy			= (s32)y;			\
	Refresh					= 1;			\
} while(0)

#define	UW_SetSize(pWindow, x, y)	\
do											\
{											\
	(pWindow)->length 		= (s32)x;				\
	(pWindow)->height		= (s32)y;				\
	Refresh					= 1;				\
} while(0)

#define	UW_SetCaption(pWindow, pcaption, length)	\
do												\
{												\
	(pWindow)->caption		= (u8 *)pcaption;			\
	(pWindow)->captionlen	= (u8)length	;			\
	Refresh					= 1;					\
} while(0)

#define	UW_SetVisible(pWindow, vvisible)		\
do											\
{											\
	(pWindow)->visible		= (bool)vvisible;		\
	Refresh					= 1;				\
} while(0)

#define	UW_SetEnable(pWindow, value)		\
do													\
{													\
	(pWindow)->enable		= (bool)value;				\
	Refresh					= 1;					\
} while(0)

#define	UW_SetOnClick(pWindow, pFunction)	\
do											\
{											\
	(pWindow)->CallBacks.Click	= (void *)pFunction;	\
} while(0)

#define	UW_SetOnButtonPress(pWindow, pFunction)	\
do													\
{													\
	(pWindow)->CallBacks.ButtonPress	= pFunction;		\
} while(0)

#define	UW_SetTimer(pWindow, pFunction, Duration)				\
do																\
{																\
	(pWindow)->CallBacks.TimerCallBack.u32TimeOut = Duration;	\
	(pWindow)->CallBacks.TimerCallBack.u32CurrentTime = 0;		\
	(pWindow)->CallBacks.TimerCallBack.Timer	= pFunction;	\
} while(0)

#define	UW_SetValue(pCheckBox, vvalue)	\
do													\
{													\
	(pCheckBox)->value		= vvalue;					\
	Refresh					= 1;						\
} while(0)



extern UW_Window		UWindows;
extern UW_Window	*	pUW_Infocus;
extern bool			Refresh;

static void UW_DrawItem(UW_Window * pstrItem);
void		UW_ListBoxSetItemHeight(UW_Window *pListBox, u8 value);
void 		UW_ListBoxAddItem(UW_Window *ListBox, UW_Window *Item);
void 		UW_ListBoxRemoveItem(UW_Window * ListBox, UW_Window * Item);
UW_Window * UW_ListBoxGetActiveItem(UW_Window * ListBox);
UW_Window * UW_ListBoxGetItem(UW_Window * ListBox,u32 index);
s32 		UW_ListBoxGetIndex(UW_Window * ListBox,UW_Window * Item);

static void 	UW_DrawCircle(s32 xpos, s32 ypos, s32 r, u16 color);
static void 	UW_DrawLine(s32 x1, s32 y1, s32 x2, s32 y2,u16 color);

#ifdef	MALLOC
void 		ialloc();
char * 		UW_Malloc(unsigned long nbytes);
void 		UW_Free(char *ap);
#endif

void * UW_MemCpy(void *s1, const void *s2, int n);

void * UW_MemSet(void *s1, unsigned char val, int n);

#endif
