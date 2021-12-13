#include "mcc_generated_files/mcc.h"
#include <string.h>

#define DEBUG 1

void debugPrint(char *message) {
    #if DEBUG
    for (uint8_t i = 0; i < strlen(message); i++) {
        UART1_Write(message[i]);
    }
    UART1_Write('\n');
    #endif
}

typedef enum {
    LV,
    PRECHARGING,
    HV_ENABLED,
    DRIVE,
    FAULT
} state_t;

typedef enum {
    NONE,
    DRIVE_REQUEST_FROM_LV
} error_t;

uint8_t is_hv_requested() {
    // TODO: change to read the signal from board
    return 1;
}

uint8_t is_drive_requested() {
    // TODO: change to read the signal from board
    return 1;
}

// TODO: read up on volatile
volatile uint8_t capacitor_volts = 0;

#define HV_CAPACITOR_VOLTS 16

void main() {
    // Reset PIC18
    SYSTEM_Initialize();

    // Initial FSM state
    state_t state = LV;
    error_t error = NONE;

    while (1) {
        // Brake system plausibility device
        // Source: https://docs.google.com/presentation/d/13g97LHTdrasgGLaJyjbnsyxRWvQFCqTvmZraG-8Hbqk/edit#slide=id.gdc3a35cc73_0_10
        
        
        // Main FSM
        // Source: https://docs.google.com/document/d/1q0RL4FmDfVuAp6xp9yW7O-vIvnkwoAXWssC3-vBmNGM/edit?usp=sharing
        switch (state) {
            case LV:
                if (is_drive_requested()) {
                    // Drive switch must be off
                    state = FAULT;
                    error = DRIVE_REQUEST_FROM_LV;
                    break;
                }
                
                if (is_hv_requested()) {
                    // HV switch was flipped
                    state = PRECHARGING;
                }
                break;
            case PRECHARGING:
                if (capacitor_volts >= HV_CAPACITOR_VOLTS) {
                    state = HV_ENABLED;
                }
                break;
            case HV_ENABLED:
                // TODO: error tolerance
                if (is_drive_requested()) {
                    state = DRIVE;
                }
                break;
            case DRIVE:
                break;
            case FAULT:
                switch (error) {
                    case DRIVE_REQUEST_FROM_LV:
                        if (!is_drive_requested()) {
                            // Drive switch is flipped off again
                            state = LV;
                            error = NONE;
                        }
                        break;
                }
                break;
        }
    }
}
