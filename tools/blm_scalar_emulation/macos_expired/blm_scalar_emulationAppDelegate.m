// $Id$
//
//  blm_scalar_emulationAppDelegate.m
//  blm_scalar_emulation
//
//  Created by Thorsten Klose on 19.11.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "blm_scalar_emulationAppDelegate.h"

@implementation blm_scalar_emulationAppDelegate

#define DEBUG_MESSAGES 0


PYMIDIVirtualSource *virtualMIDI_OUT;
PYMIDIVirtualDestination *virtualMIDI_IN;

NSInteger BLM_Config;


@synthesize mainWindow;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	// install BLM (will trigger sendNoteEvent)
	[BLMInst setDelegate:self];
	[self setBLM_Config:0];	

	// MIDI
	[self setUpMIDI];

	// MIDI Out
	NSMutableString *portNameOut = [[NSMutableString alloc] init];
	[portNameOut appendFormat:@"Virtual MBHP_BLM_SCALAR Out"];
	virtualMIDI_OUT = [[PYMIDIVirtualSource alloc] initWithName:portNameOut];

	// MIDI In
	NSMutableString *portNameIn = [[NSMutableString alloc] init];
	[portNameIn appendFormat:@"Virtual MBHP_BLM_SCALAR In"];
	virtualMIDI_IN = [[PYMIDIVirtualDestination alloc] initWithName:portNameIn];
	//[virtualMIDI_IN addReceiver:self];

	// MIDI In/Out popup
    [self buildMIDIInputPopUp];
    [self buildMIDIOutputPopUp];
    [[NSNotificationCenter defaultCenter]
	 addObserver:self selector:@selector(midiSetupChanged:)
	 name:@"PYMIDISetupChanged" object:nil
	 ];
	
	// configure Buttons
	[buttonTriggers setDelegate:self];
	[buttonTriggers setButtonChn:0];
	[buttonTriggers setButtonCC:0x40];

	[buttonTracks setDelegate:self];
	[buttonTracks setButtonChn:0];
	[buttonTracks setButtonCC:0x41];

	[buttonPatterns setDelegate:self];
	[buttonPatterns setButtonChn:0];
	[buttonPatterns setButtonCC:0x42];
	
    // We must delay opening the drawer until the window is visible
    [preferencesDrawer performSelector:@selector(open) withObject:nil afterDelay:0];
}


/////////////////////////////////////////////////////////////////////////////
// BLM Configuration
/////////////////////////////////////////////////////////////////////////////
- (void)buildConfigurationPopUp
{
    [configurationPopup removeAllItems];
    
    [configurationPopup addItemWithTitle:@"16x16"];
    [configurationPopup addItemWithTitle:@"32x16"];
    [configurationPopup addItemWithTitle:@"64x16"];
    [configurationPopup addItemWithTitle:@"128x16"];
    [configurationPopup addItemWithTitle:@"16x8"];
    [configurationPopup addItemWithTitle:@"32x8"];
    [configurationPopup addItemWithTitle:@"64x8"];
    [configurationPopup addItemWithTitle:@"16x4"];
    [configurationPopup addItemWithTitle:@"32x4"];
    [configurationPopup addItemWithTitle:@"64x4"];
	
    [configurationPopup selectItemAtIndex:BLM_Config];
}

// This is called when the user selects a new configuration from the popup
- (IBAction)configurationChanged:(id)sender
{
	[self setBLM_Config:[configurationPopup indexOfSelectedItem]];
}

- (NSInteger)BLM_Config
{
	return BLM_Config;
}

- (void)setBLM_Config:(NSInteger)config
{
	BLM_Config = config;

	switch( config ) {
		case 1: [BLMInst setColumns:32 rows:16]; break;
		case 2: [BLMInst setColumns:64 rows:16]; break;
		case 3: [BLMInst setColumns:128 rows:16]; break;
		case 4: [BLMInst setColumns:16 rows:8]; break;
		case 5: [BLMInst setColumns:32 rows:8]; break;
		case 6: [BLMInst setColumns:64 rows:8]; break;
		case 7: [BLMInst setColumns:16 rows:4]; break;
		case 8: [BLMInst setColumns:32 rows:4]; break;
		case 9: [BLMInst setColumns:64 rows:4]; break;
		default:
			BLM_Config = 0;
			[BLMInst setColumns:16 rows:16];
	}

	NSRect mainWindowFrame = [mainWindow frame];
	NSRect BLMFrame = [BLMInst frame];
	
	// center BLM
	mainWindowFrame.size = BLMFrame.size;
	mainWindowFrame.size.width = (BLMFrame.size.width < 350) ? 350 : (BLMFrame.size.width + 20);
	mainWindowFrame.size.height = BLMFrame.size.height + 60;
	BLMFrame.origin.x = (mainWindowFrame.size.width-BLMFrame.size.width) / 2;
	BLMFrame.origin.y = (mainWindowFrame.size.height-BLMFrame.size.height-20) / 2;

	// add offset for control buttons at left side
	BLMFrame.origin.x += 100;
	// increase window width
	mainWindowFrame.size.width += 100;
	
	[BLMInst setFrame:BLMFrame];
	[mainWindow setFrame:mainWindowFrame display:YES];
#if 0
	// really useful?
	[mainWindow setMinSize:mainWindowFrame.size];
	[mainWindow setMaxSize:mainWindowFrame.size];
#endif

	// send the new Layout via SysEx
	[self sendBLMLayout];

	// reconfigure PopUp
	[self buildConfigurationPopUp];
}


/////////////////////////////////////////////////////////////////////////////
// MIDI In/Out Combo Boxes
/////////////////////////////////////////////////////////////////////////////

- (void)buildMIDIInputPopUp
{
    PYMIDIManager*	manager = [PYMIDIManager sharedInstance];
    NSArray*		sources;
    NSEnumerator*	enumerator;
    PYMIDIEndpoint*	input;
	
    [midiInputPopup removeAllItems];
    
    sources = [manager realSources];
    enumerator = [sources objectEnumerator];
    while (input = [enumerator nextObject]) {
        [midiInputPopup addItemWithTitle:[input displayName]];
        [[midiInputPopup lastItem] setRepresentedObject:input];
    }
    
    if ([sources count] > 0) {
        [[midiInputPopup menu] addItem:[NSMenuItem separatorItem]];
    }
    
    [midiInputPopup addItemWithTitle:[virtualMIDI_IN name]];
    [[midiInputPopup lastItem] setRepresentedObject:virtualMIDI_IN];
    
    if ([self midiInput] == nil) {
        if ([sources count] > 0)
            [self setMIDIInput:[sources objectAtIndex:0]];
        else
            [self setMIDIInput:virtualMIDI_IN];
    }
    
    [midiInputPopup selectItemAtIndex:[midiInputPopup indexOfItemWithRepresentedObject:[self midiInput]]];
}

- (void)buildMIDIOutputPopUp
{
    PYMIDIManager*	manager = [PYMIDIManager sharedInstance];
    NSArray*		destinations;
    NSEnumerator*	enumerator;
    PYMIDIEndpoint*	output;
	
    [midiOutputPopup removeAllItems];
    
    destinations = [manager realDestinations];
    enumerator = [destinations objectEnumerator];
    while (output = [enumerator nextObject]) {
        [midiOutputPopup addItemWithTitle:[output displayName]];
        [[midiOutputPopup lastItem] setRepresentedObject:output];
    }
    
    if ([destinations count] > 0) {
        [[midiOutputPopup menu] addItem:[NSMenuItem separatorItem]];
    }
    
    [midiOutputPopup addItemWithTitle:[virtualMIDI_OUT name]];
    [[midiOutputPopup lastItem] setRepresentedObject:virtualMIDI_OUT];
    
    if ([self midiOutput] == nil) {
        if ([destinations count] > 0)
            [self setMIDIOutput:[destinations objectAtIndex:0]];
        else
            [self setMIDIOutput:virtualMIDI_OUT];
    }
    
    [midiOutputPopup selectItemAtIndex:[midiOutputPopup indexOfItemWithRepresentedObject:[self midiOutput]]];
}



// This is called when the user selects a new MIDI input from the popup
- (IBAction)midiInputChanged:(id)sender
{
    [self setMIDIInput:[[midiInputPopup selectedItem] representedObject]];

	// send Layout via SysEx
	[self sendBLMLayout];
}

// This is called when the user selects a new MIDI output from the popup
- (IBAction)midiOutputChanged:(id)sender
{
    [self setMIDIOutput:[[midiOutputPopup selectedItem] representedObject]];

	// send Layout via SysEx
	[self sendBLMLayout];
}


- (void)midiSetupChanged:(NSNotification*)notification
{
    [self buildMIDIInputPopUp];
    [self buildMIDIOutputPopUp];
}



/////////////////////////////////////////////////////////////////////////////
// MIDI IO
/////////////////////////////////////////////////////////////////////////////

- (void)setUpMIDI
{
    currentMIDI_IN = nil;
    currentMIDI_OUT = nil;
}


- (PYMIDIEndpoint*)midiInput
{
    return currentMIDI_IN;
}

- (PYMIDIEndpoint*)midiOutput
{
    return currentMIDI_OUT;
}


- (void)setMIDIInput:(PYMIDIEndpoint*)endpoint
{
    [currentMIDI_IN removeReceiver:self];
    [currentMIDI_IN autorelease];
    currentMIDI_IN = [endpoint retain];
    [currentMIDI_IN addReceiver:self];
}

- (void)setMIDIOutput:(PYMIDIEndpoint*)endpoint
{
    currentMIDI_OUT = [endpoint retain];
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Event Senders
/////////////////////////////////////////////////////////////////////////////
- (void)sendNoteEvent_Chn:(NSInteger)chn key:(NSInteger)key velocity:(NSInteger)velocity
{
#if DEBUG_MESSAGES
	NSLog(@"sendNoteEvent %02x %02x %02x\n", 0x90 | chn, key, velocity);
#endif
	Byte event[3];
	event[0] = (unsigned char)(0x90 | chn);
	event[1] = (unsigned char)key;
	event[2] = (unsigned char)velocity;
	
	MIDIPacketList packetList;
	MIDIPacket *packet = MIDIPacketListInit(&packetList);
	
	packet = MIDIPacketListAdd(&packetList, sizeof(packetList), packet,
							   0, // timestamp
							   3, event);
	[currentMIDI_OUT addSender:self];
	[currentMIDI_OUT processMIDIPacketList:&packetList sender:self];
	[currentMIDI_OUT removeSender:self];
}


- (void)sendCCEvent_Chn:(NSInteger)chn cc:(NSInteger)cc value:(NSInteger)value;
{
#if DEBUG_MESSAGES
	NSLog(@"sendCCEvent %02x %02x %02x\n", 0xb0 | chn, cc, value);
#endif
	Byte event[3];
	event[0] = (unsigned char)(0xb0 | chn);
	event[1] = (unsigned char)cc;
	event[2] = (unsigned char)value;
	
	MIDIPacketList packetList;
	MIDIPacket *packet = MIDIPacketListInit(&packetList);
	
	packet = MIDIPacketListAdd(&packetList, sizeof(packetList), packet,
							   0, // timestamp
							   3, event);
	[currentMIDI_OUT addSender:self];
	[currentMIDI_OUT processMIDIPacketList:&packetList sender:self];
	[currentMIDI_OUT removeSender:self];
}


- (void)sendBLMLayout
{
	Byte sysex[11];
	sysex[0] = 0xf0;
	sysex[1] = 0x00;
	sysex[2] = 0x00;
	sysex[3] = 0x7e;
	sysex[4] = 0x4e; // MBHP_BLM_SCALAR ID
	sysex[5] = 0x00; // Device ID 00
	sysex[6] = 0x01; // Command #1 (Layout Info)
	sysex[7] = [BLMInst numberOfColumns] & 0x7f;
	sysex[8] = [BLMInst numberOfRows];
	sysex[9] = [BLMInst numberOfLEDColours];
	sysex[10] = 0xf7;

	MIDIPacketList packetList;
	MIDIPacket *packet = MIDIPacketListInit(&packetList);
	
	packet = MIDIPacketListAdd(&packetList, sizeof(packetList), packet,
							   0, // timestamp
							   11, sysex);
	[currentMIDI_OUT addSender:self];
	[currentMIDI_OUT processMIDIPacketList:&packetList sender:self];
	[currentMIDI_OUT removeSender:self];
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Event Receiver
/////////////////////////////////////////////////////////////////////////////
- (void)processMIDIPacketList:(const MIDIPacketList*)packets sender:(id)sender
{
	// from http://svn.notahat.com/simplesynth/trunk/AudioSystem.m
    int						i, j;
    const MIDIPacket*		packet;
    Byte					message[256];
    int						messageSize = 0;
    
    
    // Step through each packet
    packet = packets->packet;
    for (i = 0; i < packets->numPackets; i++) {
        for (j = 0; j < packet->length; j++) {
            if (packet->data[j] >= 0xF8) continue;				// skip over real-time data
            
            // Hand off the packet for processing when the next one starts
            if ((packet->data[j] & 0x80) != 0 && messageSize > 0) {
                [self handleMIDIMessage:message ofSize:messageSize];
                messageSize = 0;
            }
            
            message[messageSize++] = packet->data[j];			// push the data into the message
        }
        
        packet = MIDIPacketNext (packet);
    }
    
    if (messageSize > 0)
        [self handleMIDIMessage:message ofSize:messageSize];
}

- (void)handleMIDIMessage:(Byte*)message ofSize:(int)size
{
	Byte event_type = message[0] >> 4;
	Byte chn = message[0] & 0xf;
	Byte evnt1 = message[1];
	Byte evnt2 = message[2];
	
	int LED_State;
	if( evnt2 == 0 )
		LED_State = 0;
	else if( evnt2 < 0x40 )
		LED_State = 1;
	else if( evnt2 < 0x60 )
		LED_State = 2;
	else
		LED_State = 3;

	switch( event_type ) {
		case 0x8: // Note Off
			evnt2 = 0; // handle like Note On with velocity 0
		case 0x9:
#if DEBUG_MESSAGES
			NSLog(@"Received Note Chn %d %02x %02x\n", chn+1, evnt1, evnt2);
#endif
			if( evnt1 < [BLMInst numberOfColumns] ) {
				int row = chn;
				int column = evnt1;
				[BLMInst setLED_State:column:row:LED_State];
			}
			break;

		case 0xb: {
#if DEBUG_MESSAGES
			NSLog(@"Received CC Chn %d %02x %02x\n", chn+1, evnt1, evnt2);
#endif
			switch( evnt1 ) {
				case 0x40:
					[buttonTriggers setLED_State:LED_State];
					break;
					
				case 0x41:
					[buttonTracks setLED_State:LED_State];
					break;
					
				case 0x42:
					[buttonPatterns setLED_State:LED_State];
					break;
					
				default: {
					int pattern = evnt2 | ((evnt1&1)<<7);
					int row = chn;
					int column_offset = 0;
					int state_mask = 0;
			
					if( (evnt1 & 0xf0) == 0x10 ) { // green LEDs
						state_mask = (1 << 0);
						column_offset = 8*((evnt1%16)/2);
					} else if( (evnt1 & 0xf0) == 0x20 ) { // red LEDs
						state_mask = (1 << 1);
						column_offset = 8*((evnt1%16)/2);
					}

					// change the colour of 8 LEDs if the appr. CC has been received
					if( state_mask > 0 ) {
						int column;
						for(column=0; column<8; ++column) {
							int led_mask = (pattern & (1 << column)) ? state_mask : 0;
							int _column = column+column_offset;
							int state = [BLMInst LED_State:_column:row];
							int new_state = (state & ~state_mask) | led_mask;
							[BLMInst setLED_State:_column:row:new_state];
						}
					}
				}
			}
		} break;

		case 0xf: {
			// in the hope that SysEx messages will always be sent in a single packet...
			if( size >= 8 &&
			   message[0] == 0xf0 &&
			   message[1] == 0x00 &&
			   message[2] == 0x00 &&
			   message[3] == 0x7e &&
			   message[4] == 0x4e && // MBHP_BLM_SCALAR
			   message[5] == 0x00  // Device ID
			   ) {
#if DEBUG_MESSAGES
				NSLog(@"Received Command: %02x\n", message[6]);
#endif
				if( message[6] == 0x00 && message[7] == 0x00 ) {
					// no error checking... just send layout (the hardware version will check better)
					[self sendBLMLayout];
				}
			}
		} break;
	}
}


/////////////////////////////////////////////////////////////////////////////
// Misc.
/////////////////////////////////////////////////////////////////////////////

- (IBAction)displayAbout:(id)sender
{
    [[NSWorkspace sharedWorkspace]
	 openFile:[[NSBundle mainBundle] pathForResource:@"License" ofType:@"html"]
	 ];
}


- (IBAction)visitWebSite:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://www.uCApps.de/mbhp_blm_scalar_emulation.html"]];
}


- (IBAction)sendFeedback:(id)sender
{
    NSString* name = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"];
    name = [[name componentsSeparatedByString:@" "] componentsJoinedByString:@"%20"];
	
    NSString* version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
	version = [[version componentsSeparatedByString:@" "] componentsJoinedByString:@"%20"];
	
    [[NSWorkspace sharedWorkspace] openURL:[NSURL
											URLWithString:[NSString
														   stringWithFormat:@"mailto:tk@midibox.org?subject=%@%%20%@", name, version
														   ]
											]];
}

@end
