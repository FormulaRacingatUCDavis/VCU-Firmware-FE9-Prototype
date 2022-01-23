#include "mcc_generated_files/mcc.h"

#include <string.h>
#include <time.h>

// States

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

// Breadboard components

// Switches
// On the breadboard: 0 when flipped left, 1 when flipped right

uint8_t is_hv_requested() {
    return IO_RB2_GetValue();
}

uint8_t is_drive_requested() {
    return IO_RB3_GetValue();
}

// Pedals
// On the breadboard, the range of values is 0 to 4095

#define PEDAL_MAX 4095

uint16_t get_brake_pedal_value() {
    return ADCC_GetSingleConversion(channel_ANB0);
}

uint16_t get_gas_pedal_value() {
    return ADCC_GetSingleConversion(channel_ANB1);
}

// Precharging state variables
#define MAX_CONSERVATION_SECS 4

// High voltage state variables
#define DRIVE_REQ_DELAY_MS 1000

// Initial FSM state
state_t state = LV;
error_t error = NONE;

// Used for precharging conservative timer
unsigned int conservative_timer_secs = 0;
bool is_precharging_initialized = false;

const char* state_names[] = {"LV", "PRECHARGING", "HV_ENABLED", "DRIVE", "FAULT"};
const char* error_names[] = {"NONE", "DRIVE_REQUEST_FROM_LV", "CONSERVATIVE_TIMER_MAXED", "BRAKE_NOT_PRESSED"};

void change_state(const state_t new_state) {
    // Handle edge cases
    if (state == FAULT && new_state != FAULT) {
        // Reset the error cause when exiting fault state
        error = NONE;
    }
    
    if (state == PRECHARGING && new_state != PRECHARGING) {
        // Reset flag for next time we precharge the car
        is_precharging_initialized = false;
    }
    
    // Print state transition
    printf("%s -> %s\n", state_names[state], state_names[new_state]);
    
    state = new_state;
}

void report_fault(error_t _error) {
    change_state(FAULT);
    
    printf("Error: %s\n", error_names[_error]);
    
    // Cause of error
    error = _error;
}

void main() {
    // Reset PIC18
    SYSTEM_Initialize();

    // Set up ADCC for reading analog signals
    ADCC_DischargeSampleCapacitor();

#if 1
    while (1)
    {
//        int potentiometer_val = ADCC_GetSingleConversion(channel_ANB0);
//        printf("%d\n", potentiometer_val);
        
        int switch_val = IO_RB3_GetValue();
        printf("Switch with macro: %d\n", switch_val);
        
//        switch_val = is_drive_requested();
//        printf("Switch with function: %d\n", switch_val);
    }
#endif
    
    while (1) {
        // Main FSM
        // Source: https://docs.google.com/document/d/1q0RL4FmDfVuAp6xp9yW7O-vIvnkwoAXWssC3-vBmNGM/edit?usp=sharing
        switch (state) {
            case LV:
                if (is_drive_requested()) {
                    // Drive switch should not be enabled during LV
                    report_fault(DRIVE_REQUEST_FROM_LV);
                    break;
                }

                if (is_hv_requested()) {
                    // HV switch was flipped
                    // Start charging the car to high voltage state
                    change_state(PRECHARGING);
                }
                break;
            case PRECHARGING:
                if (conservative_timer_secs >= MAX_CONSERVATION_SECS) {
                    // Precharging timed out
                    // Took too long to charge up
                    report_fault(CONSERVATIVE_TIMER_MAXED);
                    break;
                }
                     
                // TODO: get signal from motor controller
                // that capacitor volts exceeded threshold
                if (1) {
                    // Finished charging to HV in timely manner
                    change_state(HV_ENABLED);
                    break;
                }
                
                // TODO: update the timer using clock cycles instead
                __delay_ms(1000);
                conservative_timer_secs += 1;
                break;
            case HV_ENABLED:
                if (!is_hv_requested()) {
                    // Driver flipped off HV switch
                    // TODO: or capacitor voltage went under threshold
                    change_state(LV);
                    break;
                }

                if (is_drive_requested()) {
                    // Driver just flipped drive switch
                    // Give driver some time to press brake
                    __delay_ms(DRIVE_REQ_DELAY_MS);
                    
                    // Now check
                    if (get_brake_pedal_value() == PEDAL_MAX) {
                        // Brake pressed
                        change_state(DRIVE);                        
                    } else {
                        // Brake not pressed
                        report_fault(BRAKE_NOT_PRESSED);
                    }
                }
                break;
            case DRIVE:
                // TODO
                break;
            case FAULT:
                switch (error) {
                    case DRIVE_REQUEST_FROM_LV:
                        if (!is_drive_requested()) {
                            // Drive switch was flipped off
                            // Revert to LV
                            change_state(LV);
                        }
                        break;
                    case CONSERVATIVE_TIMER_MAXED:
                        if (!is_hv_requested() && !is_drive_requested()) {
                            // Drive and HV switch must both be reset
                            // to revert to LV
                            change_state(LV);
                        }
                        break;
                    case BRAKE_NOT_PRESSED:
                        if (!is_drive_requested()) {
                            // Ask driver to reset drive switch and try again
                            change_state(HV_ENABLED);
                        }
                        break;
                }
                break;
        }
    }
}
