/*
    Functionality
        A WSPR beacon with PA control on D7.
        Configuration is possible via the USB port.
        If the LCD is present the GPS seconds, status, satellites and HDOP is shown.
        A PA can be controlled on/off on D7.

    Hardware
        RFzero™ board
        PC with a terminal program 9600 Baud 8N1 <NewLine> e.g. Termite (Windows) og CuteCom (Linux)
        TX LED = RF indicator
        PPS LED = GPS PPS indicator
        Valid LED = Valid GPS indicator
        Optionoal LCD
        Optional PA control on D7

    Software
        Arduino IDE 1.8.10
        RFzero™ library www.rfzero.net/installation

        This program consists of these files all located in the same directory
            WSPR.ino
            commands.cpp and commands.h
            config.cpp and config.h
            global.cpp and glocal.h

    Version
        0.9.3
		This example program is part of the RFzero "modes" library.

    Copyright
        RFzero™ by 5Q7J, OZ2M, OZ2XH and OZ5N
		Published under GNU GPLv2
        www.rfzero.net
*/

// Arduino includes
#include <Arduino.h>


// Include the I2C LCD library
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C LCD(0x27, 20, 4); // LCD I2C addr. is 0x27, 20 chars and 4 lines and connected to D8 (SDA) and D9 (SCL)

#include <RTCZero.h>                           // Must be included for RTC use
RTCZero rtc;                                   // Create the rtc object

// RFzero include and object creation
#include <RFzero.h>        // MUST ALWAYS be included for RFzero
#include <RFzero_modes.h>  // Only relevant if CW or a MGM is needed
#include <RFzero_util.h>   // Adds some general purpose functions

// Program includes. Located in the same directory as this .ino file
#include "global.h"
#include "config.h"
#include "commands.h"

#define pinPA 7            // PA on pin

// Local variables
bool firstCalibationSaved = false;

struct gpsData gpsInfo;

// LCD update function triggered by GPS data parsed
void Display_Update()
{
    static int lastFreq = -1;
    static int lastSeconds = -1;
    static int lastMinutes = -1;
    static int lastSatellites = -1;

    static int trust = 0;
    struct gpsData gpsInfo;          // Create local object with all parameters

    int hh = rtc.getHours();
    int mm = rtc.getMinutes();
    int ss = rtc.getSeconds();
    
    if (lastMinutes != mm) {   // Even or odd minute
      LCD.setCursor(0, 3); // String #3
      gpsNMEA.getFrameData(&gpsInfo);
      // EVEN Minutes event
      if (mm % 2) {
        // Every ODD minute, Sync GPS to RTC
        if (gpsInfo.valid) {
          rtc.setTime(gpsInfo.utcHours, gpsInfo.utcMinutes, gpsInfo.utcSeconds);
          goodRTC = 1;
          trust = 30; // We trust to RTC for the next 30*2 minutes (hour)
        } else {
          if((--trust) < 0)
            trust = goodRTC = 0; 
        }
      }
      
      // Print string number 3
      if (gpsInfo.valid) {
        if (lastSatellites != gpsInfo.satellites) {
          if(seqn>=0 && gpsInfo.satellites<99) {
            sprintf(esc, "GPS:OK,SAT:%02d,SEQ:%02d", gpsInfo.satellites, seqn); // GPS:OK,SAT:99,SEQ:00
            esc[20] = '\0'; // Just in Case
            LCD.print(esc);
          }
          lastSatellites = gpsInfo.satellites;
        } 
      } else
        LCD.print("GPS: LOST SIGNAL    ");

      lastMinutes = mm;
    }

    // Display UTC time hh:mm:ss
    if (lastSeconds != ss) {  // Even or odd minute
      lastSeconds = ss;
      LCD.setCursor(12, 0);
      sprintf(esc, "%02d:%02d:%02d", hh, mm, ss);
      LCD.print(esc);
    }

    // Print Frequency
    if (lastFreq != frequency) {
      LCD.setCursor(5, 2); // String 3, Position 6. Freq
      float z = frequency/1000.0;
      LCD.print(z, 2);
      LCD.print(" kHz");
      lastFreq = frequency;
    }
    
}

// USB and GPS receivers woven into standard Arduino function. Always include in "slow" loops
void yield()
{
    static unsigned int usbBufCount = 0;
    static char usbBuf[100];

    bool keep_going ;

    // USB receiver
    do {
        keep_going = false ;

        if (SerialUSB.available())
        {
            char ch = (char) SerialUSB.read();

            if (usbBufCount < sizeof(usbBuf) - 1)
            {
                if (ch == '\n')
                {   // End of line found so parse the buffer
                    ParseCommand(usbBuf);

                    usbBufCount = 0;   // Clear buffer
                    usbBuf[0] = 0;
                }
                else
                {   // Pad buffer with latest char
                    usbBuf[usbBufCount] = ch;
                    usbBufCount++;
                    usbBuf[usbBufCount] = 0;
                }
            }
            else
            {   // Overflow so reset buffer
                usbBufCount = 0;
                usbBuf[0] = 0;
            }
            keep_going = true ;
        }

        // GPS receiver, also initiates the display updating if relevant
        if (gps.autoParse()) {
            //gpsNMEA.getFrameData(&gpsdata) ;

            if (gpsEcho) {
                const char* frame = gpsNMEA.getLastFrame() ;
                SerialUSB.println(frame);
            }

            if (displayMode & displayAutoUpdate) {
                Display_Update();
            }
        }

    } while (keep_going) ;
}

void setup()
{
    // Initialize RFzero for the specific application (must always be present)
    RFzero.Init(EEPROM_TYPE_24LC08B);                // EEPROM types: http://www.rfzero.net/documentation/eeprom-data/

    // Heat up
    si5351a.rfOff();
    si5351a.setFrequency(200000000.0);

    // Setup USB
    SerialUSB.begin(9600);
    delay(2000);  // Delay used instead of "while (!(SerialUSB));" because it halts if not connected to USB. Checks show up to 1 s so 2 s to be on the safe side
    
    // Initialize the RTC
    rtc.begin();

    // Load configuration
    if (eeprom.isUnconfig())
    {
        SerialUSB.println("\nEEPROM is unconfigured. Entering config mode");
        SerialUSB.println("Enter   ?   to see commands\n");
        ParseCommand((char*)"config");
        while (configMode)                                // Wait until initial configuration is completed before continuing
            yield();
    }
    else
    {   // Print welcome message on USB
        PrintLibPrgVer(0);
        SerialUSB.println("Enter   ?   for help");
        SerialUSB.println("RFzero>");
    }
    LoadConfiguration();                                  // Load defaults incl. default xtal frequency

    // Setup PA control
    pinMode(0, OUTPUT);                                   // D0 is PA control
    digitalWrite(0, LOW);

    // Prepare display
    if (displayMode)
    {
        //LCD.begin(16, 2);
        LCD.begin();
        LCD.noCursor();
        LCD.clear();
        
        // Print S/W versions
        LCD.print("RFzero library");
        LCD.setCursor(0, 1);
        LCD.print("version: ");
        LCD.print(RFZERO_LIBRARY_VERSION);
        delay(3000);

        LCD.clear();
        sprintf(esc, "%.16s", swPackage);
        LCD.print(esc);
        LCD.setCursor(0, 1);
        LCD.print("version: ");
        LCD.print(swVersion);
        delay(3000);
        LCD.clear();

        //Prepare the LCD
        
        // First string
        LCD.print("RFzero WSPR"); // Print it at 0,0

        // Second string
        LCD.setCursor(0, 1);
        sprintf(esc, "MSG: %s %s", call, locator);
        LCD.print(esc); 
        
        // Third string
        LCD.setCursor(0, 2);
        LCD.print("QRG: ");
        float z = frequency/1000.0;
        LCD.print(z, 2);
        LCD.print(" kHz");
        
        // Fourth string
        LCD.setCursor(0, 3);
        LCD.print("STABILIZING...");
        
        displayAutoUpdate = 1;
    }

    // Warm up?
    if (warmUp)
    {
        char buf[40];
        sprintf(buf, "Warming up for %d s. Please wait", warmUp);
        SerialUSB.print(buf);
        for (int i = 0; i < warmUp; i++)
        {
            SerialUSB.print(".");
            for (int j = 0; j < 5; j++)
            {
                hardware.txLed(ON);
                delay(100);
                hardware.txLed(OFF);
                delay(100);
            }
        }
        if (configMode)
            SerialUSB.println("\nRFzero config>");
        else
            SerialUSB.println("\nRFzero>");
    }

    // Always wait for GPS to be valid
    while (!gpsNMEA.getValid())
        yield();

    pinMode(pinPA, OUTPUT);                                   // Set PA pin mode
    digitalWrite(pinPA, LOW);                                 // Turn PA pin off
    
    // Center the PLL
    Modes.setCWCarrierTone(true);
    
}

void loop()
{
    // Took-took GPS
    struct gpsData gpsInfo;          // Create local object with all parameters
    
    // Calibrate if required and save first time
    if (--calibIntervalCounter < 1)
    {
        calibIntervalCounter = calibInterval;
        si5351a.refreshRefFrequency();

        // Already saved first measured reference frequency to EEPROM?
        if (!firstCalibationSaved)
        {
            RFzero.saveReferenceStartFreq();
            firstCalibationSaved = true;
        }
        Modes.calculateTones(frequency);
    }

    //Get Time From RTC, validate it, validate minutes/seconds, starte the transmission
    if(goodRTC && !rtc.getSeconds()) {
      if (!(rtc.getMinutes() % 2)) {
        //sprintf(esc, "UTC: %02d:%02d:%02d\n", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
        sprintf(esc, "TX:%02d:%02d", rtc.getHours(), rtc.getMinutes());
        LCD.setCursor(12, 0);
        LCD.print(esc);
        SerialUSB.println(esc);
        
        digitalWrite(pinPA, HIGH);                    // Turn PA pin on
                
        si5351a.rfOn();
        Modes.sendMGM();
        si5351a.rfOff();                              // Send "no signal"

        hardware.txLed(OFF);
        
        digitalWrite(pinPA, LOW);                     // Turn PA pin off
        
        if(++seqn > 99) seqn = 0;                     // just calculate number of seuences we sent out
      }
    }        
}



// ----------------- EOF -------------------------------------------------------------------
