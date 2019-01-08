/*
 * SysEx Parser
 *
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#import <mios32.h>
#import <externalheaders.h>
#import <sysex.h>
#import <app.h>
#import <display.h>
#import <control.h>

/////////////////////////////////////////////////////////////////////////////
// Internal Prototypes
/////////////////////////////////////////////////////////////////////////////

void SYSEX_SendAck(u8 ack_code, u8 ack_arg);
void SYSEX_Cmd(u8 cmd_state, u8 midi_in);
void SYSEX_GetDataBytes(u8 cmd_state, u8 midi_in);
void SYSEX_Cmd_Ping(u8 cmd_state, u8 midi_in);
void SYSEX_DumpBuffer(void);
void SYSEX_SendCommand(u8 type, bool doubleClick, bool tripleClick, bool longClick, u8 control, u8* arguments, u8 length);

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

mios32_midi_port_t sysex_port = DEFAULT;
sysex_state_t	sysex_state;					//state of sysex communication

u8 bit_ctr7; //counts the bits of midi_in
u8 value8; //processing buffer for one byte of the decoded data
u8 bit_ctr8; //counts through the bits of value8

int data_ctr;  //counts the received 8-bit bytes
u8 data_buffer[128];				//byte buffer for storing the data
u8* data_ptr;				//pointer to the buffer. we use this one in the code to be able to change it in order to write directly to other locations in memory
ab_command_header_t* cmd_header; //pointer to the first three bytes of the buffer

u8 control; //for storing the control of a message
u8 type; //for storing the type of a message                 

/////////////////////////////////////////////////////////////////////////////
// Static definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
void SYSEX_Init(void)
{
	sysex_port = DEFAULT;
	sysex_state.ALL = 0;
	//set the struct pointer to the buffer
	cmd_header = (ab_command_header_t*)data_buffer;

	// install SysEx parser
	MIOS32_MIDI_SysExCallback_Init(SYSEX_Parser);
	
#if DEBUG_VERBOSE_LEVEL >= 1
	MIOS32_MIDI_SendDebugMessage("SYSEX_Init:FINISHED");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx command to the host
/////////////////////////////////////////////////////////////////////////////
void SYSEX_SendCommand(u8 type, bool doubleClick, bool tripleClick, bool longClick, u8 control, u8* arguments, u8 length)
{
#if DEBUG_VERBOSE_LEVEL >= 5
	MIOS32_MIDI_SendDebugMessage("SYSEX_SendCommand:START");
#endif

#if DEBUG_VERBOSE_LEVEL >= 4
	MIOS32_MIDI_SendDebugMessage("sending command %02x, value %d", type, *arguments);
#endif

	u8 buffer[16]; //buffer for holding the assembled data
	u8* buffer_ptr = buffer+3; 
	ab_command_header_t* send_cmd_header;
	send_cmd_header = (ab_command_header_t*)buffer;
	
	// send sysex header
	MIOS32_MIDI_SendSysEx(DEFAULT, (u8* const)ab_sysex_header, sizeof(ab_sysex_header));	
	
  //send three command bytes
	send_cmd_header->commandType = type;
	send_cmd_header->tripleClick = tripleClick;
	send_cmd_header->doubleClick = doubleClick;
	send_cmd_header->longClick = longClick;
	send_cmd_header->device = MIOS32_MIDI_DeviceIDGet() & 0x3F; //only use the first six bits
	send_cmd_header->allDevices = 0;
	send_cmd_header->allControls = 0;
	send_cmd_header->controlNumber = control;

	//append the arguments to the buffer
	u8 i;
	for (i=0;i<length;i++) {
		*buffer_ptr++ = *arguments++; //faster than indexed array copying
	}
	//send the stuff
	SYSEX_SendData(buffer, length+3);

  // send footer
	MIOS32_MIDI_SendSysEx(DEFAULT, (u8* const)&ab_sysex_footer, 1);	

#if DEBUG_VERBOSE_LEVEL >= 5
	MIOS32_MIDI_SendDebugMessage("SYSEX_SendCommand:FINISHED");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// This function looks which command is being received
/////////////////////////////////////////////////////////////////////////////
void SYSEX_GetCommand(void)
{
	if( cmd_header->device == MIOS32_MIDI_DeviceIDGet() || cmd_header->allDevices ) {

		switch( type ) {
			case AB_CMD_ACK:
				SYSEX_SendCommand(AB_CMD_ACK, FALSE, FALSE, FALSE, 0, NULL, 0);
				break;
			case AB_CMD_RESET:
				MIOS32_SYS_Reset();
				break;
			case AB_CMD_SENDCONFIG:
				DISPLAY_SendConfig();
				break;
			case AB_CMD_FLASH:
				if (cmd_header->allControls) {
					DISPLAY_LedCheck();
				} else {
					DISPLAY_Flash(control, (display_led_color_t*)&data_buffer[3]);
				}
				break;
			case AB_CMD_SHORTFLASH:
				DISPLAY_MeterFlash(control, (display_led_color_t*)&data_buffer[3]);
				break;
			case AB_CMD_BLINK:
				DISPLAY_SetBlink(control, data_buffer[3]);
				break;
			case AB_CMD_SETCOLOR:
				DISPLAY_SetColor(control, (display_led_color_t*)&data_buffer[3]);
				break;
			case AB_CMD_CLIP_SET:
			case AB_CMD_CLIP_RUN:
			case AB_CMD_CLIP_RERUN:
			case AB_CMD_CLIP_START:
			case AB_CMD_CLIP_STOP:
				CONTROL_ClipHandler(control, type);
				break;
			case AB_CMD_PREFERENCES:
				DISPLAY_SetBrightness(data_buffer[3]);
				CONTROL_setIntervals(&data_buffer[4]);
				break;
			case AB_CMD_VALUE:
				CONTROL_SetValue(cmd_header->controlNumber, data_buffer[3]);
				break;
			default:
				break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for SysEx messages
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Parser(u8 midi_in)
{		
#if DEBUG_VERBOSE_LEVEL >= 5 && DEBUG_VERBOSE_LEVEL < 9 
	MIOS32_MIDI_SendDebugMessage("SYSEX_Parser: BEGIN");
#endif
	// branch depending on state
	if( !sysex_state.MY_SYSEX ) {
		if( midi_in != ab_sysex_header[sysex_state.CTR] ) {
			// incoming byte doesn't match, reset sysex state
			sysex_state.ALL = 0;
		} else {
			if( ++sysex_state.CTR == sizeof(ab_sysex_header) ) {
				// complete header received, waiting for data				
				sysex_state.MY_SYSEX = 1;
				
				// disable merger forwarding until end of sysex message
				//MIOS_MPROC_MergerDisable();
				
				//we are starting to receive: clear all counters
				value8 = 0;
				bit_ctr8 = 0;
				data_ctr = 0;
				//initialize the buffer pointer to the beginning of our array
				data_ptr = data_buffer;
				
				MIOS32_MIDI_SendDebugMessage("sys begin");
			}
		}
	} else {
		// check for end of SysEx message or invalid status byte
		if( midi_in >= 0x80 ) {
			if( midi_in == ab_sysex_footer ) {
				SYSEX_GetDataBytes(SYSEX_STATE_END, midi_in);
				MIOS32_MIDI_SendDebugMessage("sys end");
				
#if DEBUG_VERBOSE_LEVEL >= 4 && DEBUG_VERBOSE_LEVEL < 9
				MIOS32_MIDI_SendDebugMessage("SYSEX_Parser: Finished receiving message");					
#endif													
			}
		} else {
			SYSEX_GetDataBytes(SYSEX_STATE_GET, midi_in);
		}
	}
#if DEBUG_VERBOSE_LEVEL >= 5 && DEBUG_VERBOSE_LEVEL < 9 
	MIOS32_MIDI_SendDebugMessage("SYSEX_Parser: END");
#endif	

	return 1; // don't forward package to APP_MIDI_NotifyPackage()
}


/////////////////////////////////////////////////////////////////////////////
// Receive the data bytes
/////////////////////////////////////////////////////////////////////////////
void SYSEX_GetDataBytes(u8 state, u8 midi_in)
{
	u8 i;
	u8 tmp;
	
#if DEBUG_VERBOSE_LEVEL >= 5  && DEBUG_VERBOSE_LEVEL < 9
	MIOS32_MIDI_SendDebugMessage("SYSEX_Cmd_GetDataBytes: BEGIN");
#endif
	
	switch( state ) {
			
		case SYSEX_STATE_GET: {
			//do some unscrambling
			for(bit_ctr7=0; bit_ctr7<7; ++bit_ctr7) {
				value8 = (value8 << 1) | ((midi_in & 0x40) ? 1 : 0);
				midi_in <<= 1;			
				if( ++bit_ctr8 >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 4  && DEBUG_VERBOSE_LEVEL < 9
					MIOS32_MIDI_SendDebugMessage("SYSEX_Cmd_GetDataBytes: Received data byte %02d", value8);
#endif					
					*data_ptr = value8;
					data_ptr++;
					data_ctr++;
					bit_ctr8 = 0;
					
					//check if we are receiving a control after the first three bytes. if yes, write the sysex data directly to the control
					if (data_ctr == AB_CMD_HEADER_LENGTH) {
						control = cmd_header->controlNumber;
						type = cmd_header->commandType;
#if DEBUG_VERBOSE_LEVEL >= 4 && DEBUG_VERBOSE_LEVEL < 9
						MIOS32_MIDI_SendDebugMessage("SYSEX_Cmd_GetDataBytes: Is receiving message with type 0x%02x, device %02d and control %02d", cmd_header->commandType, cmd_header->device, cmd_header->controlNumber);
#endif									
						if (type == AB_CMD_RECEIVECONTROL) {
#if DEBUG_VERBOSE_LEVEL >= 4 && DEBUG_VERBOSE_LEVEL < 9
							MIOS32_MIDI_SendDebugMessage("SYSEX_Cmd_GetDataBytes: Receiving Control Data, changing data_ptr to write into Control %02d", cmd_header->controlNumber);
#endif					
							//Disable Led Task and inputs
							DISPLAY_SuspendLedTask();
							CONTROL_SetInputState(0);

							//set the receiving buffer to be the control itself
							data_ptr = (u8*)&controls[control];
							//MIOS32_MIDI_SendDebugMessage("ctrl %d", control);
						}
					}
					
					//check if we are receiving another control
					if ((data_ctr == AB_CMD_HEADER_LENGTH + AB_CMD_RECEIVECONTROL_DATA_SIZE + 1) && (type == AB_CMD_RECEIVECONTROL)) {
						CONTROL_UpdateControl(control);
						//we are at the beginning of a new control, reset the counters
						data_ctr = AB_CMD_HEADER_LENGTH; //next comes the data
						--data_ptr; //data_ptr has been increased already, set it back
						control = *data_ptr; //data_ptr points now to the last byte written, which is the control number
						data_ptr = (u8*)&controls[control]; //set the receiving buffer to be the control itself
						//MIOS32_MIDI_SendDebugMessage("nextctrl %d", control);
					}
				}
			}
			
			break;		
		}
		default: { // SYSEX_STATE_END			
			if (type == AB_CMD_RECEIVECONTROL) {
#if DEBUG_VERBOSE_LEVEL >= 4 && DEBUG_VERBOSE_LEVEL < 9
				MIOS32_MIDI_SendDebugMessage("SYSEX_Cmd_GetDataBytes: Received Control Data, setting changesteps");
				MIOS32_MIDI_SendDebugMessage("SYSEX_Cmd_GetDataBytes: type and value of targeted control: %02d %02d", controls[cmd_header->controlNumber].Type, controls[cmd_header->controlNumber].Value);
				MIOS32_MIDI_SendDebugMessage("SYSEX_Cmd_GetDataBytes: maincolor value of targeted control: %02d %02d %02d", controls[cmd_header->controlNumber].mainColor.Red, controls[cmd_header->controlNumber].mainColor.Green, controls[cmd_header->controlNumber].mainColor.Blue);
#endif			
				//TODO: if received a control, turn on inputs for this control
				CONTROL_UpdateControl(control);
				
				//turn on led task and inputs again
				DISPLAY_ResumeLedTask();
				CONTROL_SetInputState(1);

			} else {
				SYSEX_GetCommand();
			}
			//reset the whole sysex state
			sysex_state.ALL = 0;
		}
	}
	
#if DEBUG_VERBOSE_LEVEL >= 5 && DEBUG_VERBOSE_LEVEL < 9
	MIOS32_MIDI_SendDebugMessage("SYSEX_Cmd_GetDataBytes: FINISHED");
#endif
	
}

/////////////////////////////////////////////////////////////////////////////
// This function scrambles one 8-bit byte array into 7bit and sends it in steps of 8 7-bit bytes (representing 7 8-bit bytes)
/////////////////////////////////////////////////////////////////////////////
void SYSEX_SendData(u8* array8, u8 length)
{	
	u8 array7[8]; 
	u8 array7counter = 0;
	u8 value7 = 0;
	u8 bit_ctr7 = 0;
	u8 array8counter;
	u8 value8;
	u8 bit_ctr8;
	
	//go through the whole array
	for(array8counter=0; array8counter<length; array8counter++,array8++) {
		value8 = *array8;
		for(bit_ctr8=0; bit_ctr8<8; ++bit_ctr8) {
			value7 = (value7 << 1) | ((value8 & 0x80) ? 1 : 0);
			value8 <<= 1;
			if( ++bit_ctr7 >= 7 ) {
				array7[array7counter] = value7;
				array7counter++;
				value7 = 0;
				bit_ctr7 = 0;
			}
			//is the array7 full?
			if (array7counter == 8) {
				//send the array and reset it
				MIOS32_MIDI_SendSysEx(DEFAULT, array7, 8);
				array7counter = 0;
			}
		}
	}	
	//are bits left in the value7?
	if( bit_ctr7 ) {
		(value7 <<= (7-bit_ctr7));
		array7[array7counter] = value7; 
		array7counter++;
	}
	//do we have bytes left in array7?
	if (array7counter) {
		//send the remaining bytes
		MIOS32_MIDI_SendSysEx(DEFAULT, array7, array7counter);
	}
}
