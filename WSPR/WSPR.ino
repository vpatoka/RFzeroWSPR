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

#define INTERVAL  4        // Interval in minutes between of transmissions NOTE: Minus 110.6 seconds. So, if its value 4, then actual interval will be 2

#define encA A0
#define encB A1
#define encP A2  // Rotary encoder push pin

volatile int encState = 0;

const uint8_t encTableHalfStep[6][4] =
{
    {0x3, 0x2, 0x1, 0x0},
    {0x23, 0x0, 0x1, 0x0},
    {0x13, 0x2, 0x0, 0x0},
    {0x3, 0x5, 0x4, 0x0},
    {0x3, 0x3, 0x4, 0x10},
    {0x3, 0x5, 0x3, 0x20},
};

const uint8_t encTableFullStep[7][4] =
{
    {0x0, 0x2, 0x4,  0x0},
    {0x3, 0x0, 0x1, 0x10},
    {0x3, 0x2, 0x0,  0x0},
    {0x3, 0x2, 0x1,  0x0},
    {0x6, 0x0, 0x4,  0x0},
    {0x6, 0x5, 0x0, 0x20},
    {0x6, 0x5, 0x4,  0x0},
};


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
    static int txseconds = 0;

    int hh = rtc.getHours();
    int mm = rtc.getMinutes();
    int ss = rtc.getSeconds();
    
    if (lastMinutes != mm) {   // Even or odd minute
      if (--TXflag < 0) TXflag = INTERVAL;
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
      lastMinutes = mm;
    }

    // Display UTC time hh:mm:ss
    if (lastSeconds != ss) {  // Even or odd minute
      lastSeconds = ss;
      LCD.setCursor(12, 0);
      if(digitalRead(pinPA) == LOW) {
        sprintf(esc, "%02d:%02d:%02d", hh, mm, ss);
        txseconds = 0;
      } else
        sprintf(esc, "TX: %03d ", txseconds++);
        
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
    
    // Print string number 3
    if (gpsInfo.valid || digitalRead(pinPA) == HIGH) {
      if(lastSatellites != gpsInfo.satellites || digitalRead(pinPA) == HIGH) {
        if(gpsInfo.satellites<99) {
          LCD.setCursor(0, 3);
          sprintf(esc, "GPS:OK,SAT:%02d,SEQ:%02d", gpsInfo.satellites, seqn); // GPS:OK,SAT:99,SEQ:00
          LCD.print(esc);
        }
        lastSatellites = gpsInfo.satellites;
      }  
    } else {
      if(!goodRTC) {
        LCD.setCursor(0, 3);
        LCD.print("GPS: LOST SIGNAL    ");
      }  
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

    // Rotary Encoder
    pinMode(encA, INPUT); // Setup pins for encoder
    pinMode(encB, INPUT);
    pinMode(encP, INPUT);
    digitalWrite(encA, HIGH);
    digitalWrite(encB, HIGH);
    digitalWrite(encP, HIGH);

    // Setup interrupt handling for the encoder half- or full-step
    attachInterrupt(digitalPinToInterrupt(encA), RotEncFullStep, CHANGE);
    //attachInterrupt(digitalPinToInterrupt(encA), RotEncHalfStep, CHANGE);

    attachInterrupt(digitalPinToInterrupt(encB), RotEncFullStep, CHANGE);
    //attachInterrupt(digitalPinToInterrupt(encB), RotEncHalfStep, CHANGE);

    attachInterrupt(digitalPinToInterrupt(encP), RotEncPushButton, LOW);

    //manageFreq();

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

        manageFreq();
        displayAutoUpdate = 1;
        ScreenSet();
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
    if(!TXflag && goodRTC) {
      TXflag = INTERVAL;
      if (!(rtc.getMinutes() % 2) && !rtc.getSeconds()) {
        /* DEBUG 
        #sprintf(esc, "TX:%02d:%02d", rtc.getHours(), rtc.getMinutes());
        #LCD.setCursor(12, 0);
        #LCD.print(esc);
        #SerialUSB.println(esc);
        */
        digitalWrite(pinPA, HIGH);                    // Turn PA pin on
                
        si5351a.rfOn();
        Modes.sendMGM();
        si5351a.rfOff();                              // Send "no signal"

        hardware.txLed(OFF);
        
        digitalWrite(pinPA, LOW);                     // Turn PA pin off
        
        if(++seqn > 99) seqn = 1;                     // just calculate number of seuences we sent out
      }
    }        
}

void RotEncHalfStep()
{
  encState = encTableHalfStep[encState & 0xF][(digitalRead(encB) << 1) | digitalRead(encA)];
  int result = encState & 0x30;
  if (result == 0x20)
    REdec = 1;
  if (result == 0x10)
    REinc = 1;  
  //if (result)
  //  SerialUSB.println(result == 0x20 ? "Left" : "Right");  // 0x20 = Left, 0x10 = Right
}

void RotEncFullStep()
{
  encState = encTableFullStep[encState & 0xF][(digitalRead(encB) << 1) | digitalRead(encA)];
  int result = encState & 0x30;
  if (result == 0x20)
    REdec = 1;
  if (result == 0x10)
    REinc = 1; 
  //if (result)   
  //SerialUSB.println(result == 0x20 ? "Left" : "Right");  // 0x20 = Left, 0x10 = Right
}

void RotEncPushButton()
{
  int timeStamp = millis();
  static int lastTimeStamp = 0;
  if (timeStamp - lastTimeStamp > 1) {
    if(--REbutton < 0) REbutton = 7;
    //SerialUSB.println("Push button pressed");
  }  
  lastTimeStamp = timeStamp;
}

void manageFreq(void)
{
  int prevstep = 8;
  double prevfreq = 0.0;
  double mult = 0.0;
  int flag = 0;

  LCD.clear();
  LCD.print("Set Beacon Frequency");
  LCD.setCursor(0, 1);
  LCD.print("   Push or Rotate");
  LCD.setCursor(0, 2);
  LCD.print("MULT: x");  
  LCD.setCursor(0, 3);
  LCD.print("FREQ: ");  

  while(REbutton) {
    delay(200);
    if(flag++ > 300) // Wait for any activity for 1minute. if not - continue with default
      return;
    switch(REbutton) {
      case 1:
        mult = 1.0;
        break;
      case 2:
        mult = 10.0;
        break;
      case 3:
        mult = 100.0;
        break;
      case 4:
        mult = 1000.0;
        break;
      case 5:
        mult = 10000.0;
        break;
      case 6:
        mult = 100000.0;
        break;
      case 7:
        mult = 1000000.0;
        break;
      default:
        mult = 0.0;
        break;
    }

    if(REbutton != prevstep) {
      flag = 0;
      prevstep = REbutton;
      LCD.setCursor(7, 2);
      LCD.print("       ");
      LCD.setCursor(7, 2);
      LCD.print(mult, 0);  
    }   
    if(REinc) {
      if ((frequency += (REinc*mult)) > 200000000.0)
        frequency = 2605.0;
      REinc = 0;
    } else { 
      if(REdec) {
        if ((frequency -= (REdec*mult)) < 2605.0)
          frequency = 200000000.0;
        REdec = 0;
      }
    }
    if(prevfreq != frequency) {
      flag = 0;
      prevfreq = frequency;
      LCD.setCursor(6, 3);
      LCD.print(frequency, 0);
    }   
  }
}

void ScreenSet(void)
{
  LCD.clear();
        
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
}
// ----------------- EOF -------------------------------------------------------------------
