#ifndef	_UW_CHRSET_H_
#define	_UW_CHRSET_H_
extern		UW_Window	CharSetWindow;

#define		CHAR_A_INDEX		0
#define		CHAR_a_INDEX		26
#define		CHAR_1_INDEX		52
#define		CHAR_PLUS_INDEX		62


#define		CHAR_GROUP			&chargroups[0]
#define		CHAR_PREV_POS		CHAR_GROUP+0
#define		CHAR_CAPS_POS		CHAR_GROUP+2
#define		CHAR_ALPHA_POS		CHAR_GROUP+4
#define		CHAR_NUM_POS		CHAR_GROUP+6
#define		CHAR_CHR_POS		CHAR_GROUP+8
#define		CHAR_BKSPC_POS		CHAR_GROUP+10
#define		CHAR_SPACE_POS		CHAR_GROUP+13
#define		CHAR_NEXT_POS		CHAR_GROUP+16
#define		CHAR_OK_POS			CHAR_GROUP+19
#define		CHAR_CANCEL_POS		CHAR_GROUP+21

#define		CHAR_SET_LENGTH		72
#define		CHAR_PER_ROW		5
#define		BTN_LENGTH			25
#define		BTN_HEIGHT			25
#endif 
