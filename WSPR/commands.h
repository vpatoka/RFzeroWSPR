#ifndef _COMMANDS_H
#define _COMMANDS_H

// Prototypes
extern uint8_t configMode;                                 // Indicates if program is in config mode
extern uint8_t configChanged;                              // Indicates if the configuration has changed

void ParseCommand(char *str);

#endif  // _COMMANDS_H

// ----------------- EOF -------------------------------------------------------------------
