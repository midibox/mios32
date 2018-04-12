/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern u8 screen[64][128];             // Screen buffer [y][x]

// If showLogo is true, draw the MBLoopa Logo (usually during unit startup)
void screenShowLoopaLogo(u8 showLogo);

// Set the currently selected clip
void screenSetClipSelected(u8 clipNumber);

// Set the currently recorded-to clip
void screenSetClipRecording(u8 clipNumber, u8 recordingActiveFlag);

// Set the length of a clip
void screenSetClipLength(u8 clipNumber, u16 stepLength);

// Set the position of a clip
void screenSetClipPosition(u8 clipNumber, u16 stepPosition);

// Set the global song step position (e.g. for displaying the recording-clip step)
void screenSetSongStep(u32 stepPosition);

// Flash a short-lived message to the center of the screen
void screenFormattedFlashMessage(const char* format, ...);

// Set scene change notification message (change in ticks)
void screenSetSceneChangeInTicks(u8 ticks);

// Notify, that a screen page change has occured (flash a page descriptor for a while)
void screenNotifyPageChanged();

// Display the current screen buffer
void display();

// Render test screen, one half is "full on" for flicker tests
void testScreen();
