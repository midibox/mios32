// $Id$
//
//  blm_scalar_emulationAppDelegate.h
//  blm_scalar_emulation
//
//  Created by Thorsten Klose on 19.11.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <PYMIDI/PYMIDI.h>
#import "BLM.h"

// Will only work with SDK 10.6
//@interface blm_scalar_emulationAppDelegate : NSObject <NSApplicationDelegate> {
@interface blm_scalar_emulationAppDelegate : NSObject {
    NSWindow *mainWindow;
	
	IBOutlet BLM *BLMInst;

	IBOutlet NSDrawer* preferencesDrawer;
	IBOutlet NSPopUpButton* midiInputPopup;
	IBOutlet NSPopUpButton* midiOutputPopup;
	IBOutlet NSPopUpButton* configurationPopup;

	// MIDI related
	PYMIDIEndpoint*	currentMIDI_IN;
	PYMIDIEndpoint*	currentMIDI_OUT;
		
}

@property (assign) IBOutlet NSWindow *mainWindow;	

- (void)buildConfigurationPopUp;
- (IBAction)configurationChanged:(id)sender;
- (NSInteger)BLM_Config;
- (void)setBLM_Config:(NSInteger)config;

- (void)buildMIDIInputPopUp;
- (void)buildMIDIOutputPopUp;
- (IBAction)midiInputChanged:(id)sender;
- (IBAction)midiOutputChanged:(id)sender;
- (void)midiSetupChanged:(NSNotification*)notification;
	
- (void)sendNoteEvent:(NSInteger)chn:(NSInteger)key:(NSInteger)velocity;
- (void)sendBLMLayout;

- (void)setUpMIDI;
- (PYMIDIEndpoint*)midiInput;
- (void)setMIDIInput:(PYMIDIEndpoint*)endpoint;
- (PYMIDIEndpoint*)midiOutput;
- (void)setMIDIOutput:(PYMIDIEndpoint*)endpoint;
- (void)processMIDIPacketList:(const MIDIPacketList*)packets sender:(id)sender;
- (void)handleMIDIMessage:(Byte*)message ofSize:(int)size;

- (IBAction)displayAbout:(id)sender;
- (IBAction)visitWebSite:(id)sender;
- (IBAction)sendFeedback:(id)sender;
	
@end
