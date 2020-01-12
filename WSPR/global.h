#ifndef _GLOBAL_H
#define _GLOBAL_H

// Arduino includes
#include <Arduino.h>

// RFzero and RFzero_modes includes
#include <RFzero.h>
#include <RFzero_modes.h>

// S/W package info
extern const char swPackage[];
extern const char swVersion[];

// Hardware
extern int displayMode;                // LCD mode: 0: none, 1: 16x2
extern int displayAutoUpdate;          // Allow automated updating of the LCD

extern int warmUp;                     // Warm up seconds counter

// GPS
extern int gpsEcho;                    // Echo GPS data to management port

// LCD
extern char esc[32];                   // Used to create one LCD string (20 char long)

// SEQ
extern int seqn;                       // Number of sequnce to send (roll out at 99)

// GOOD RTC
extern int goodRTC;                    // Special flag to trust to RTC

// Beacon data
extern double frequency;               // The nominal normal beacon frequency, i.e. carrier frequency
extern int calibInterval;              // Si5351A reference frequency calibration interval
extern int calibIntervalCounter;       // Running calibration counter
extern char call[16];                  // Call sign. Make sure [15] = 0 or earlier
extern char locator[9];                // Locator. Make sure [8] = 0 or earlier

// WSPR
extern int wsprPower;                  // The WSPR power level in dBm 

extern Modulate Modes;

// Function prototypes
void PrintLibPrgVer(const int captionType);

#endif // _GLOBAL_H

// ----------------- EOF -------------------------------------------------------------------
