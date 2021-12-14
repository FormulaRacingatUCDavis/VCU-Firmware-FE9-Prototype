#include "mcc_generated_files/mcc.h"

#include <string.h>
#include <time.h>

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
    DRIVE_REQUEST_FROM_LV,
    CONSERVATIVE_TIMER_MAXED,
    BRAKE_NOT_PRESSED
} error_t;

uint8_t is_hv_requested() {
    // TODO: change to read the signal from board
    return 1;
}

uint8_t is_drive_requested() {
    // TODO: change to read the signal from board
    return 1;
}

volatile uint8_t capacitor_volts = 0;

#define HV_CAPACITOR_VOLTS 16

#define MAX_CONSERVATION_SECS 4

#define DRIVE_REQ_DELAY_MS 1000

// Initial FSM state
state_t state = LV;
error_t error = NONE;

// Used for precharging conservative timer
double conservative_timer_secs;
clock_t last_iteration_time;
bool is_precharging_initialized = false;

// HV
double errorTolerance;

void main() {
    // Reset PIC18
    SYSTEM_Initialize();

    while (1) {
        // Main FSM
        // Source: https://docs.google.com/document/d/1q0RL4FmDfVuAp6xp9yW7O-vIvnkwoAXWssC3-vBmNGM/edit?usp=sharing
        switch (state) {
            case LV:
                if (is_drive_requested()) {
                    // Drive switch cannot be enabled during LV
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
                if (!is_precharging_initialized) {
                    // Initialize conservative timer
                    conservative_timer_secs = 0;                    
                    is_precharging_initialized = true;
                } else {
                    // Update conservative timer
                    double elapsed = (clock() - last_iteration_time) / CLOCKS_PER_SEC;
                    conservative_timer_secs += elapsed;
                }

                // Use this to find elapsed time from now to next iteration
                last_iteration_time = clock();
                
                if (conservative_timer_secs >= MAX_CONSERVATION_SECS) {
                    // Precharging timed out
                    state = FAULT;
                    error = CONSERVATIVE_TIMER_MAXED;
                    // Reset flag
                    is_precharging_initialized = false;
                    break;
                }
                                
                if (capacitor_volts >= HV_CAPACITOR_VOLTS) {
                    // Finished charging to HV
                    state = HV_ENABLED;
                    // Reset flag
                    is_precharging_initialized = false;
                }

                break;
            case HV_ENABLED:
                if (is_drive_requested()) {
                    // Wait for driver to press brake
                    __delay_ms(DRIVE_REQ_DELAY_MS);
                    if (errorTolerance == 0) {
                        // Brake pressed
                        state = DRIVE;                        
                    } else {
                        // Brake not pressed
                        state = FAULT;
                        error = BRAKE_NOT_PRESSED;
                    }
                }
                
                if (!is_hv_requested()) {
                    // Driver disabled HV
                    // or capacitor voltage went under threshold
                    state = LV;
                }
                break;
            case DRIVE:
                break;
            case FAULT:
                switch (error) {
                    case DRIVE_REQUEST_FROM_LV:
                        if (!is_drive_requested()) {
                            // Drive switch is flipped off again
                            // Revert to LV like normal
                            state = LV;
                            error = NONE;
                        }
                        break;
                    case CONSERVATIVE_TIMER_MAXED:
                        if (!is_hv_requested() && !is_drive_requested()) {
                            // Drive and HV switch must both be reset
                            // to revert to LV
                            state = LV;
                            error = NONE;
                        }
                        break;
                    case BRAKE_NOT_PRESSED:
                        if (!is_drive_requested()) {
                            // Ask driver to reset drive switch and try again
                            state = HV_ENABLED;
                            error = NONE;
                        }
                        break;
                }
                break;
        }
    }
}
