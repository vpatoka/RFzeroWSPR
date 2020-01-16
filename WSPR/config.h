#ifndef _CONFIG_H
#define _CONFIG_H

// Enable RFzero class
#include "RFzero_modes.h"

// EEPROM MAP
// HARDWARE
#define EEPROM_HW_RefStartFreq        16  // 4 bytes
#define EEPROM_HW_T1                  21  // 1 byte
#define EEPROM_HW_DisplayMode         22  // 1 byte
#define EEPROM_HW_WarmUp              24  // 1 byte

// GPS
#define EEPROM_GPS_Wait               33  // 1 byte
#define EEPROM_GPS_Echo               34  // 1 byte

// COMMON
#define EEPROM_COMMON_Locator        101  // 8+1 bytes

// BEACON (100-199 is commen for all beacon programs)
#define EEPROM_BEACON_Frequency      128  // 8 bytes
#define EEPROM_BEACON_CalibInterval  137  // 1 byte
#define EEPROM_BEACON_Call           140  // 15+1 bytes
#define EEPROM_BEACON_MGMMsg         156  // 15+1 bytes

// WSPR
#define EEPROM_BEACON_WSPRPower      194  // 1 byte

// INTERVAL
#define EEPROM_BEACON_Interval       195  // 1 byte

// Function prototypes
void LoadConfiguration();

#endif // _CONFIG_H

// ----------------- EOF -------------------------------------------------------------------
