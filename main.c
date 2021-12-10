#include "mcc_generated_files/mcc.h"

#define DEBUG 0

#if DEBUG
void printLine(char *message) {
    for (uint8_t i = 0; i < strlen(message); i++) {
        UART1_Write(message[i]);
    }
    UART1_Write('\n');
}
#endif

typedef enum {
    START,
    LV,
    PRECHARGING,
    HV_ENABLED,
    DRIVE,
    FAULT
} state_t;

void main(void) {
    // Reset PIC18
    SYSTEM_Initialize();

    // Initial FSM state
    state_t state = START;
    
    while (1) {
        // Main FSM
        // Source: https://docs.google.com/document/d/1q0RL4FmDfVuAp6xp9yW7O-vIvnkwoAXWssC3-vBmNGM/edit?usp=sharing
        switch (state) {
            case START:
                // TODO: Initialize CAN?
                // TODO: Add other stuff
                state = LV;
                break;
            case LV:
                //  TODO: Add other stuff and so forth...
                break;
            case PRECHARGING:
                break;
            case HV_ENABLED:
                break;
            case DRIVE:
                break;
            case FAULT:
                break;
        }
    }
}
