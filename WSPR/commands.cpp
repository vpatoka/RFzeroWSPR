// RFzero includes
#include <RFzero.h>
#include <RFzero_modes.h>
#include <RFzero_util.h>

// Program includes. Located in the same directory as the .ino file
#include "global.h"
#include "config.h"
#include "commands.h"

uint8_t configMode = 0;                                          // Indicates if program is in config mode
uint8_t configChanged = 0;                                       // Indicates if the configuration has changed

void ParseCommand(char *str)
{
    const uint8_t NONE = 5;
    uint8_t comStatus = NONE;
    int value, value1;
    double freq;
    char buffreq[20];
    char buf[80];

    TrimCharArray(str);
    if (strlen(str))
    {
        comStatus = 0;
        SerialUSB.println(str);

        // *** Configuration control ***
        // config
        if (!configMode)
        {
            if (0 == strncmp("config", str, sizeof("config") - 1))
            {
                configMode = 1;
                comStatus = 1;
                gpsEcho = 0;  // Turn GPS echo off when in configuration mode
                PrintLibPrgVer(0);
            }
            else if ((0 == strncmp("help", str, sizeof("help") - 1)) || (0 == strncmp("?", str, sizeof("?") - 1)))
            {
                PrintLibPrgVer(0);

                if (!configMode)  //  In run mode
                    SerialUSB.println("Enter   config   to enter configuration mode");

                comStatus = 1;
            }
        }

        // In configuration mode
        else
        {
            // exit
            if (0 == strncmp("exit", str, sizeof("exit") - 1))
            {
                configMode = 0;
                comStatus = NONE;
                gpsEcho = eeprom.readByte(EEPROM_GPS_Echo, 0);

                if (configChanged)
                {
                    SerialUSB.println("Configuration changed, please wait");
                    LoadConfiguration();
                    configChanged = 0;
                }
            }


            // HARDWARE ..........................................................
            // wr frst FREQ
            else if (0 == strncmp("wr frst ", str, sizeof("wr frst ") - 1))
            {
                if (1 == sscanf(&str[sizeof("wr frst ") - 1], "%d", &value))
                {
                    if ((26990000L <= value) && (value <= 27010000L))        // Accept only frequencies +/- 10 kHz from 27 MHz
                    {
                        eeprom.writeInteger(EEPROM_HW_RefStartFreq, value);
                        comStatus = 1;
                        configChanged = 1;
                    }
                    else
                        comStatus = 3;
                }
                else
                    comStatus = 2;
            }


            // wr hw T1 LCD
            else if (0 == strncmp("wr hw ", str, sizeof("wr hw ") - 1))
            {
                if (2 == sscanf(&str[sizeof("wr hw ") - 1], "%d %d", &value, &value1))
                {
                    if ((0 <= value) && (value <= 2) && (0 <= value1) && (value1 <= 4))  // T1 and LCD
                    {
                        eeprom.writeByte(EEPROM_HW_T1, value);
                        eeprom.writeByte(EEPROM_HW_DisplayMode, value1);
                        comStatus = 1;
                        configChanged = 1;
                    }
                    else
                        comStatus = 2;
                }
                else
                    comStatus = 2;
            }


            // wr warmup DURATION
            else if (0 == strncmp("wr warmup ", str, sizeof("wr warmup ") - 1))
            {
                if (1 == sscanf(&str[sizeof("wr warmup ") - 1], "%d", &value))
                {
                    if ((0 <= value) && (value <= 0xFF))
                    {
                        eeprom.writeByte(EEPROM_HW_WarmUp, value);
                        comStatus = 1;
                        configChanged = 1;
                    }
                    else
                        comStatus = 2;
                }
                else
                    comStatus = 2;
            }


            // GPS PARAMETERS ..........................................................
            // wr echo ONOFF
            else if (0 == strncmp("wr echo ", str, sizeof("wr echo ") - 1))
            {
                if (1 == sscanf(&str[sizeof("wr echo ") - 1], "%d", &value))
                {
                    if ((0 <= value) && (value <= 1))
                    {
                        eeprom.writeByte(EEPROM_GPS_Echo, value);
                        comStatus = 1;
                        configChanged = 1;
                    }
                    else
                        comStatus = 2;
                }
                else
                    comStatus = 2;
            }


            // BEACON PARAMETERS ..........................................................
            // wr defaults
            else if (0 == strncmp("wr defaults", str, sizeof("wr defaults") - 1))
            {
                // HARDWARE
                eeprom.writeInteger(EEPROM_HW_RefStartFreq, 27000000L);
                eeprom.writeByte(EEPROM_HW_T1, 0);
                eeprom.writeByte(EEPROM_HW_DisplayMode, 1);
                eeprom.writeByte(EEPROM_HW_WarmUp, 0);

                // GPS
                eeprom.writeByte(EEPROM_GPS_Echo, 0);

                // BEACON
                eeprom.writeByte(EEPROM_BEACON_CalibInterval, 5);
                eeprom.writeByte(EEPROM_BEACON_Interval, 2);

                // WSPR
                eeprom.writeByte(EEPROM_BEACON_WSPRPower, 13);

                comStatus = 1;
                configChanged = 1;
            }


            // wr freq FREQ
            else if (0 == strncmp("wr freq ", str, sizeof("wr freq ") - 1))
            {
                if (1 == sscanf(&str[sizeof("wr freq ") - 1], "%s", buf))
                {
                    double fr = strtod(buf, NULL);
                    if (100000.0 <= fr)                                  // Accept only frequencies from 100 kHz and up
                    {
                        eeprom.writeDouble(EEPROM_BEACON_Frequency, fr);
                        comStatus = 1;
                        configChanged = 1;
                    }
                    else
                        comStatus = 3;
                }
                else
                    comStatus = 2;
            }


            // wr cal INTERVAL
            else if (0 == strncmp("wr cal ", str, sizeof("wr cal ") - 1))
            {
                if (1 == sscanf(&str[sizeof("wr cal ") - 1], "%d", &value))
                {
                    if ((1 <= value) && (value <= 0xFF))
                    {
                        eeprom.writeByte(EEPROM_BEACON_CalibInterval, value);
                        comStatus = 1;
                        configChanged = 1;
                    }
                    else
                        comStatus = 2;
                }
                else
                    comStatus = 2;
            }

            // wr txdly INTERVAL_MINUTES
            else if (0 == strncmp("wr txdly ", str, sizeof("wr txdly ") - 1))
            {
                if (1 == sscanf(&str[sizeof("wr txdly ") - 1], "%d", &value))
                {
                    if ((1 <= value) && (value <= 0xFF))
                    {
                        eeprom.writeByte(EEPROM_BEACON_Interval, value);
                        comStatus = 1;
                        configChanged = 1;
                    }
                    else
                        comStatus = 2;
                }
                else
                    comStatus = 2;
            }


            // wr bcn CALL
            else if (0 == strncmp("wr bcn ", str, sizeof("wr bcn ") - 1))
            {
                if (1 == sscanf(&str[sizeof("wr bcn ") - 1], "%s", buf))
                {
                    if (strlen(buf) < 16)
                    {
                        for (uint32_t i = 0; i < 16; i++)
                        {
                            if (i < strlen(buf))
                            {
                                eeprom.writeByte(EEPROM_BEACON_Call + i, toupper(buf[i]));
                            }
                            else
                            {
                                eeprom.writeByte(EEPROM_BEACON_Call + i, 0);            // Fill the remainder with null
                            }
                        }
                        eeprom.writeByte(EEPROM_BEACON_Call + 15, 0);            // Hard stop sz
                        comStatus = 1;
                        configChanged = 1;
                    }
                    else
                        comStatus = 2;
                }
                else
                    comStatus = 2;
            }


            // wr loc LOCATOR
            else if (0 == strncmp("wr loc ", str, sizeof("wr loc ") - 1))
            {
                if (1 == sscanf(&str[sizeof("wr loc ") - 1], "%s", buf))
                {
                    if (strlen(buf) < 9)
                    {
                        for (uint32_t i = 0; i < 9; i++)
                        {
                            if (i < strlen(buf))
                            {
                                eeprom.writeByte(EEPROM_COMMON_Locator + i, toupper(buf[i]));
                            }
                            else
                            {
                                eeprom.writeByte(EEPROM_COMMON_Locator + i, 0);    // Fill the remainder with null
                            }
                        }
                        eeprom.writeByte(EEPROM_COMMON_Locator + 8, 0);            // Hard stop sz
                        comStatus = 1;
                        configChanged = 1;
                    }
                    else
                        comStatus = 2;
                }
                else
                    comStatus = 2;
            }

            // WSPR
            // wr pwr POWER
            else if (0 == strncmp("wr pwr ", str, sizeof("wr pwr ") - 1))
            {
                if (1 == sscanf(&str[sizeof("wr pwr ") - 1], "%d", &value))
                {
                    if ((value >= 0) && (value <= 60) && ((value % 10 == 0) || ((value - 3) % 10 == 0) || ((value - 7) % 10 == 0)))
                    {
                        eeprom.writeByte(EEPROM_BEACON_WSPRPower, value);
                        comStatus = 1;
                        configChanged = 1;
                    }
                    else
                        comStatus = 2;
                }
                else
                    comStatus = 2;
            }


            // rd fref
            else if (0 == strncmp("rd fref", str, sizeof("rd fref") - 1))
            {
                double fref = freqCount.getReferenceFrequency() ;
                SerialUSB.println(fref, 2);
                comStatus = 99;
            }


            // OVERVIEW ..........................................................
            // rd cfg
            else if (0 == strncmp("rd cfg", str, sizeof("rd cfg") - 1))
            {
                PrintLibPrgVer(1);

                SerialUSB.println("Configuration");
                SerialUSB.println("=============");

                sprintf(buf, "Reference start frequency in Hz: 27 MHz*        : %d", eeprom.readInteger(EEPROM_HW_RefStartFreq, 27E6));
                SerialUSB.println(buf);
                sprintf(buf, "T1 type: 0: transformer*, 1: combiner, 2: none  : %d", eeprom.readByte(EEPROM_HW_T1, 0));
                SerialUSB.println(buf);
                sprintf(buf, "Display: 0: none, 1: 20x4*                      : %d", eeprom.readByte(EEPROM_HW_DisplayMode, 1));
                SerialUSB.println(buf);

                sprintf(buf, "Warm up before transmitting: 0* to 255 s        : %d", eeprom.readByte(EEPROM_HW_WarmUp, 0));
                SerialUSB.println(buf);

                // GPS
                sprintf(buf, "\nEcho GPS data to USB port: 0: no*, 1: yes       : %d", eeprom.readByte(EEPROM_GPS_Echo, 0));
                SerialUSB.println(buf);

                // BEACON
                freq = eeprom.readDouble(EEPROM_BEACON_Frequency, 0.0);
                doubleToString(freq, 0, buffreq);
                sprintf(buf, "\nNominal beacon frequency in Hz                  : %s", buffreq);
                SerialUSB.println(buf);
                sprintf(buf, "TX interval (minutes) 2 to 59, 4*               : %d", eeprom.readByte(EEPROM_BEACON_Interval, 4));
                SerialUSB.println(buf);
                sprintf(buf, "Calibration interval 1 to 255, 15*              : %d", eeprom.readByte(EEPROM_BEACON_CalibInterval, 15));
                SerialUSB.println(buf);

                SerialUSB.print("Call, max six (type 1)/ten (type 2) chars.      : ");
                for (int i = 0; i < 16; i++)
                    buf[i] = (char) eeprom.readByte(EEPROM_BEACON_Call + i, '0');
                buf[15] = 0;
                SerialUSB.println(buf);
                SerialUSB.print("Locator, max AA00AA00                           : ");
                for (int i = 0; i < 9; i++)
                    buf[i] = (char) eeprom.readByte(EEPROM_COMMON_Locator + i, '0');
                buf[8] = 0;
                SerialUSB.println(buf);
                sprintf(buf, "Power level in dBm: 0, 3, 7, 10, 13*, 17 ... 60 : %d", eeprom.readByte(EEPROM_BEACON_WSPRPower, 13));
                SerialUSB.println(buf);

                SerialUSB.println("\n*: default value\n");

                comStatus = NONE;
            }

            // help   or   ?
            else if ((0 == strncmp("help", str, sizeof("help") - 1)) || (0 == strncmp("?", str, sizeof("?") - 1)))
            {
                PrintLibPrgVer(1);

                SerialUSB.println("Available MMI commands. Type   exit   to return to run mode\n");

                SerialUSB.println("Configuration");
                SerialUSB.println("=============");
                SerialUSB.println("  wr bcn CALL                to set the CALL, max six/ten characters");
                SerialUSB.println("  wr loc LOCATOR             to set the LOCATOR, max six characters");
                SerialUSB.println("  wr pwr POWER               to set the power level in dBm, 0, 3, 7, 10, 13, 17, 20, 23, 27 30 ... 60\n");

                SerialUSB.println("  wr freq FREQ               to set the beacon nominal frequency in Hz from 100 kHz and up\n");

                SerialUSB.println("  wr txdly MINUTES           to set delay between of transmissions, 2 - 59");

                SerialUSB.println("  wr cal INTERVAL            to set the number of sequences before calibrating the frequencies, 1 - 255");
                SerialUSB.println("  wr warmup SECONDS          to set the number of seconds to warm up the H/W before transmitting, 0 - 255\n");

                SerialUSB.println("  wr echo on/off             to turn on/off GPS data echo on the USB on/off: 0: off, 1: on\n");

                SerialUSB.println("  wr defaults                to set the H/W and S/W defaults");
                SerialUSB.println("  wr hw T1 LCD               to set the H/W where:");
                SerialUSB.println("                                T1  : 0: transformer   1: combiner   2: none");
                SerialUSB.println("                                LCD : 0: none          1: 20x4");
                SerialUSB.println("  wr frst FREQ               to write the Si5351A start reference frequency to the EEPROM\n");

                SerialUSB.println("  rd cfg                     to list the current configuration\n");

                comStatus = NONE;
            }
        }
    }

    switch (comStatus)
    {
        case 0: SerialUSB.println("Unknown command"); break;
        case 1: SerialUSB.println("OK"); break;
        case 2: SerialUSB.println("Invalid data"); break;
        case 3: SerialUSB.println("Invalid frequency"); break;
        case 4: SerialUSB.println("Invalid level"); break;
        default : break;  // No response since result is self explanatory
    }

    if (configMode)
        SerialUSB.println("RFzero config>");
    else
        SerialUSB.println("RFzero>");
}

// ----------------- EOF -------------------------------------------------------------------
