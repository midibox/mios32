#include "commonIncludes.h"

// --- constants ---
#define configFilePath "/setup.txt"

// --- globals ---
extern u8 configChangesToBeWritten_;

// --- functions ---
// (After a configuration change), write the global setup file to disk
extern void writeSetup();

// Read global setup from disk (usually only at startup time)
extern void readSetup();
