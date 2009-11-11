#include <mios32.h>
#include <string.h>
#include "UWindows.h"
#include "UW_CharSet.h"
UW_Window	CharSetWindow;
UW_Window	EditBox;
UW_Window	* PtrCurrentEditBox;
UW_Window	CharNext,CharPrev,CharAlpha,CharNum,CharChr,CharCaps,CharBkSpc,CharSpace;
UW_Window	BtnOk,BtnCancel;
const u8 		chargroups[] = " < A a 1 @ <-    > OKExt";
const u8 		charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890+-*/=_@%. ";
UW_Window	CharSet[CHAR_SET_LENGTH];
u8			EditCaption[100];
u8 			GroupIndex;

void CharSet_StrCat (u8 * str1, u8 len1, u8 * str2, u8 len2)
{
	u8 index;
	for (index=0;index<len2;index++)
		str1[index+len1]=str2[index];
	str1[(index+len1)]='\0';
	return;
}

void BackSpace_Click(void * pstrItem)
{
	EditBox.caption[EditBox.captionlen] = '\0';
	EditBox.captionlen--;
}
static void DrawEffectiveChars(void)
{
	u8 	index;
	u8 	x=0;
	for (index=0;index<CHAR_SET_LENGTH;index++)
	{
		UW_SetVisible(&CharSet[index],0);
	}
	x =1;
	for (index=0;index<CHAR_PER_ROW;index++)
	{		
		UW_SetPosition(&CharSet[index+GroupIndex],x,5);
		UW_SetVisible(&CharSet[index+GroupIndex],1);
		x += BTN_LENGTH;
	}		
}

void GroupSet_Click(void * pstrItem)
{
	
	UW_Window * pWindow = (UW_Window *) pstrItem;
	if (pWindow->caption[1] == 'A')
		GroupIndex = CHAR_A_INDEX;
	else if (pWindow->caption[1] == 'a')
		GroupIndex = CHAR_a_INDEX;
	else if (pWindow->caption[1] == '1')
		GroupIndex = CHAR_1_INDEX;
	else if (pWindow->caption[1] == '@')
		GroupIndex = CHAR_PLUS_INDEX;
	DrawEffectiveChars();
}

void Prev_Click(void * pstrItem)
{
	GroupIndex = ((GroupIndex-CHAR_PER_ROW) > 0) ? (GroupIndex-CHAR_PER_ROW) : 0;
	DrawEffectiveChars();
}

void Next_Click(void * pstrItem)
{
	GroupIndex = ((GroupIndex+CHAR_PER_ROW) < (CHAR_SET_LENGTH-CHAR_PER_ROW)) ? (GroupIndex+CHAR_PER_ROW) : (CHAR_SET_LENGTH-CHAR_PER_ROW);
	DrawEffectiveChars();	
}

void CharSet_Click(void * pstrItem)
{
	UW_Window * pWindow = (UW_Window *) pstrItem;
	CharSet_StrCat(EditBox.caption, EditBox.captionlen, pWindow->caption,1);
	EditBox.captionlen++;
}

void CharSet_Show(UW_Window * CurrentEditBox)
{
	memcpy(EditBox.caption,CurrentEditBox->caption,CurrentEditBox->captionlen);
	EditBox.captionlen = CurrentEditBox->captionlen;
	PtrCurrentEditBox = CurrentEditBox;
	UW_SetVisible(&CharSetWindow,1);
}

void CharSet_Hide(u8 Apply)
{
	if (Apply == 1)
	{
		memcpy(PtrCurrentEditBox->caption,EditBox.caption,EditBox.captionlen);
		PtrCurrentEditBox->captionlen = EditBox.captionlen;
	}
	UW_SetVisible(&CharSetWindow,0);
}

void BtnOkClick(UW_Window * pWindow)
{
	CharSet_Hide(1);
}

void BtnCancelClick(UW_Window * pWindow)
{
	CharSet_Hide(0);
}

void CharSet_Init(void)
{
	u8 x=0,y=0;
	u8 index;	
	UW_Add(&CharSetWindow,UW_FORM,&UWindows,NULL);
	UW_SetVisible(&CharSetWindow,0);

	for (index=0;index<CHAR_SET_LENGTH;index++)
	{
		UW_Add(&CharSet[index],UW_BUTTON,&CharSetWindow,NULL);
		UW_SetSize(&CharSet[index],BTN_LENGTH,BTN_HEIGHT);
		UW_SetCaption(&CharSet[index],&charset[index],1);
		UW_SetOnClick(&CharSet[index],CharSet_Click);
		UW_SetVisible(&CharSet[index],0);
		UW_SetPosition(&CharSet[index],1,5);
	}

	x = 1;
	y = 3*BTN_HEIGHT+20;
	UW_Add(&EditBox,UW_EDIT,&CharSetWindow,NULL);
	UW_SetSize(&EditBox,125,BTN_HEIGHT);
	UW_SetCaption(&EditBox,EditCaption,0);
	UW_SetEnable(&EditBox,0);	
	UW_SetPosition(&EditBox,x,y);

	x = 1;
	y = 2*BTN_HEIGHT+15;
	UW_Add(&BtnOk,UW_BUTTON,&CharSetWindow,NULL);
	UW_SetSize(&BtnOk,BTN_LENGTH,BTN_HEIGHT);
	UW_SetCaption(&BtnOk,CHAR_OK_POS,3);
	UW_SetOnClick(&BtnOk,BtnOkClick);	
	UW_SetPosition(&BtnOk,x,y);
	x += BTN_LENGTH;

	UW_Add(&CharPrev,UW_BUTTON,&CharSetWindow,NULL);
	UW_SetSize(&CharPrev,BTN_LENGTH,BTN_HEIGHT);
	UW_SetCaption(&CharPrev,CHAR_PREV_POS,3);
	UW_SetOnClick(&CharPrev,Prev_Click);	
	UW_SetPosition(&CharPrev,x,y);
	x += BTN_LENGTH;

	UW_Add(&CharSpace,UW_BUTTON,&CharSetWindow,NULL);
	UW_SetSize(&CharSpace,BTN_LENGTH,BTN_HEIGHT);
	UW_SetCaption(&CharSpace,CHAR_SPACE_POS,3);
	UW_SetOnClick(&CharSpace,CharSet_Click);	
	UW_SetPosition(&CharSpace,x,y);
	x += BTN_LENGTH;

	UW_Add(&CharNext,UW_BUTTON,&CharSetWindow,NULL);
	UW_SetSize(&CharNext,BTN_LENGTH,BTN_HEIGHT);
	UW_SetCaption(&CharNext,CHAR_NEXT_POS,3);
	UW_SetOnClick(&CharNext,Next_Click);	
	UW_SetPosition(&CharNext,x,y);
	x += BTN_LENGTH;
	
	
	
	UW_Add(&BtnCancel,UW_BUTTON,&CharSetWindow,NULL);
	UW_SetSize(&BtnCancel,BTN_LENGTH,BTN_HEIGHT);
	UW_SetCaption(&BtnCancel,CHAR_CANCEL_POS,3);
	UW_SetOnClick(&BtnCancel,BtnCancelClick);	
	UW_SetPosition(&BtnCancel,x,y);
	x += BTN_LENGTH;
	
	x =1;
	y = BTN_HEIGHT+10;
	UW_Add(&CharCaps,UW_BUTTON,&CharSetWindow,NULL);
	UW_SetSize(&CharCaps,BTN_LENGTH,BTN_HEIGHT);
	UW_SetCaption(&CharCaps,CHAR_CAPS_POS,3);
	UW_SetOnClick(&CharCaps,GroupSet_Click);	
	UW_SetPosition(&CharCaps,x,y);
	x += BTN_LENGTH;

	UW_Add(&CharAlpha,UW_BUTTON,&CharSetWindow,NULL);
	UW_SetSize(&CharAlpha,BTN_LENGTH,BTN_HEIGHT);
	UW_SetCaption(&CharAlpha,CHAR_ALPHA_POS,3);
	UW_SetOnClick(&CharAlpha,GroupSet_Click);	
	UW_SetPosition(&CharAlpha,x,y);
	x += BTN_LENGTH;
	
	UW_Add(&CharNum,UW_BUTTON,&CharSetWindow,NULL);
	UW_SetSize(&CharNum,BTN_LENGTH,BTN_HEIGHT);
	UW_SetCaption(&CharNum,CHAR_NUM_POS,3);
	UW_SetOnClick(&CharNum,GroupSet_Click);	
	UW_SetPosition(&CharNum,x,y);
	x += BTN_LENGTH;

	UW_Add(&CharChr,UW_BUTTON,&CharSetWindow,NULL);
	UW_SetSize(&CharChr,BTN_LENGTH,BTN_HEIGHT);
	UW_SetCaption(&CharChr,CHAR_CHR_POS,3);
	UW_SetOnClick(&CharChr,GroupSet_Click);	
	UW_SetPosition(&CharChr,x,y);
	x += BTN_LENGTH;
	
	UW_Add(&CharBkSpc,UW_BUTTON,&CharSetWindow,NULL);
	UW_SetSize(&CharBkSpc,BTN_LENGTH,BTN_HEIGHT);
	UW_SetCaption(&CharBkSpc,CHAR_BKSPC_POS,3);
	UW_SetOnClick(&CharBkSpc,BackSpace_Click);	
	UW_SetPosition(&CharBkSpc,x,y);
	x += BTN_LENGTH;

	
}
