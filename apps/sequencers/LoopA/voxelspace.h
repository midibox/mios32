// LoopA Voxelspace routines (initially used in MIOS32 Voxelspace demo done in 2011)

// Landscape/heightfield generator
void calcField(void);

// Voxel space display demo
void voxelFrame(void);

// Add a note in the voxel space
void voxelNoteOn(u8 note, u8 velocity);

// Add a note in the voxel space
void voxelNoteOff(u8 note);

// Add a tick line in the voxel space
void voxelTickLine();

// Clear current notes
void voxelClearNotes();

// Clear current voxel field
void voxelClearField();
