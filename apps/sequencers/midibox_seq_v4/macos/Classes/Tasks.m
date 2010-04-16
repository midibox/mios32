//
//  Tasks.m
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 30.06.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "Tasks.h"

#include <mios32.h>


@implementation Tasks

// local variables to bridge objects to C functions
static NSObject *_self;

// semaphores used by ../core/tasks.h
NSRecursiveLock *SDCardSemaphore;
NSRecursiveLock *MIDIINSemaphore;
NSRecursiveLock *MIDIOUTSemaphore;
NSRecursiveLock *LCDSemaphore;


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
	
	SDCardSemaphore = [[NSRecursiveLock alloc] init];
	MIDIINSemaphore = [[NSRecursiveLock alloc] init];
	MIDIOUTSemaphore = [[NSRecursiveLock alloc] init];
	LCDSemaphore = [[NSRecursiveLock alloc] init];
}


//////////////////////////////////////////////////////////////////////////////
// Semaphore access functions for C code
//////////////////////////////////////////////////////////////////////////////

void TASKS_SDCardSemaphoreTake(void) { [SDCardSemaphore lock]; }
void TASKS_SDCardSemaphoreGive(void) { [SDCardSemaphore unlock]; }

void TASKS_MIDIINSemaphoreTake(void) { [MIDIINSemaphore lock]; }
void TASKS_MIDIINSemaphoreGive(void) { [MIDIINSemaphore unlock]; }

void TASKS_MIDIOUTSemaphoreTake(void) { [MIDIOUTSemaphore lock]; }
void TASKS_MIDIOUTSemaphoreGive(void) { [MIDIOUTSemaphore unlock]; }

void TASKS_LCDSemaphoreTake(void) { [LCDSemaphore lock]; }
void TASKS_LCDSemaphoreGive(void) { [LCDSemaphore unlock]; }

@end
