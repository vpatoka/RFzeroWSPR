// Program includes. Located in the same directory as the .ino file
#include "global.h"
#include "config.h"

#include <RFzero_modes.h>

void LoadConfiguration()
{
    // EXISTING VALUES
    int oldCalibInterval = calibInterval;
    double oldFrequency = frequency;
    int oldInterval = Interval;

    // HARDWARE
    int t1Hardware = eeprom.readByte(EEPROM_HW_T1, 0);
    si5351a.rfOutputT1(t1Hardware);
    displayMode = eeprom.readByte(EEPROM_HW_DisplayMode, 1);
    warmUp = eeprom.readByte(EEPROM_HW_WarmUp, 0);


    // GPS
    gpsEcho = eeprom.readByte(EEPROM_GPS_Echo, 0);


    // COMMON
    for (int i = 0; i < 9; i++)
        locator[i] = eeprom.readByte(EEPROM_COMMON_Locator + i, 0x2e);
    locator[8] = 0;                                        // Safe 0 terminator
    

    // BEACON
    frequency = eeprom.readDouble(EEPROM_BEACON_Frequency, 28555000.0);
    if ((frequency < 100000.0) || (frequency > 298765432.0))  // In frequency is outside range then set to default and save
    {
        frequency = 28555000.0;
        eeprom.writeDouble(EEPROM_BEACON_Frequency, frequency);
    }     
    calibInterval = eeprom.readByte(EEPROM_BEACON_CalibInterval, 5);
    if (calibInterval != oldCalibInterval)
        calibIntervalCounter = 2;                          // Force recalibration

    for (int i = 0; i < 16; i++)
        call[i] = eeprom.readByte(EEPROM_BEACON_Call + i, 0x2e);
    call[15] = 0;                                          // Safe 0 terminator
    
    Interval = eeprom.readByte(EEPROM_BEACON_Interval, 4);
    if (Interval != oldInterval)
      if(Interval < 2 || Interval > 59)
        Interval = 2;                                     // Force TX Interval


    // WSPR
    wsprPower = eeprom.readByte(EEPROM_BEACON_WSPRPower, 13);
    if ((wsprPower < 0) || (wsprPower > 60) || ((wsprPower % 10 != 0) && ((wsprPower - 3) % 10 != 0) && ((wsprPower - 7) % 10 != 0)))
    {
        wsprPower = 13;                                     // Wrong power level so set it to 13 dBm
        eeprom.writeByte(EEPROM_BEACON_WSPRPower, 13);
    }

    Modes.setupWSPR(call, locator, wsprPower);

    // Recalc regs if frequency changed
    if (oldFrequency != frequency)
    {
        Modes.calculateTones(frequency) ;
        Modes.setCWCarrierTone(true) ;   // will recenter PLL
    }
}

// ----------------- EOF -------------------------------------------------------------------
