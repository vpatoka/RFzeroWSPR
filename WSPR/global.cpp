// Own include
#include "global.h"

// S/W package info
const char swPackage[] = "Beacon WSPR";
const char swVersion[] = "0.9.3 mod";

// Hardware
int displayMode = 0;                          // LCD mode: 0: none, 1: 16x2
int displayAutoUpdate = 0;                    // Allow automated updating of the display

int warmUp = 0;                               // Warm up seconds counter

// GPS
int gpsEcho = 0;                              // Echo GPS data to management port

// LCD
char esc[32];                                 // Used to create one LCD string (20 char long)

// SEQ
int seqn = -1;

// Good RTC flag
extern int goodRTC = 0;

// Beacon data
double frequency = 0.0;                       // The nominal normal beacon frequency, i.e. carrier frequency
int calibInterval = -1;                        // Si5351A reference frequency calibration interval
int calibIntervalCounter;                     // Running calibration counter
char call[16] = "               ";            // Call sign. Make sure [15] = 0 or earlier
char locator[9] = "        ";                 // Locator. Make sure [8] = 0 or earlier

// WSPR
int wsprPower = 13;                           // The WSPR power level in dBm 

class Modulate Modes;

void PrintLibPrgVer(const int captionType)
{
    switch (captionType)
    {
        case 0: break;
        case 1:
            SerialUSB.println("\nSoftware");
            SerialUSB.println("========");
            break;
        default: break;
    }
    
    char buf[50];
    sprintf(buf, "RFzero library, v.%s", RFZERO_LIBRARY_VERSION);
    SerialUSB.println(buf);
    sprintf(buf, "%s, v.%s", swPackage, swVersion);
    SerialUSB.println(buf);
    
    switch (captionType)
    {
        case 0:
            if (gpsNMEA.getValid())// gpsInfo.valid
            {
                struct gpsData gpsInfo;
                gpsNMEA.getFrameData(&gpsInfo);

                sprintf(buf, "UTC (now): %04d-%02d-%02d, %02d:%02d:%02d", gpsInfo.utcYear, gpsInfo.utcMonth, gpsInfo.utcDay, gpsInfo.utcHours, gpsInfo.utcMinutes, gpsInfo.utcSeconds);
                SerialUSB.println(buf);
            }
            break;
        case 1:
            SerialUSB.println();;
            break;
        default: break;
    }
}

// ----------------- EOF -------------------------------------------------------------------
