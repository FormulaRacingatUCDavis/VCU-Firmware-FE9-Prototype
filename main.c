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

// TODO: add sensor discrepancy error
typedef enum {
    NONE,
    DRIVE_REQUEST_FROM_LV,
    CONSERVATIVE_TIMER_MAXED,
    BRAKE_NOT_PRESSED,
    HV_DISABLED_WHILE_DRIVING
} error_t;

// Controls

// Switches
// 0 means off, 1 means on

uint8_t is_hv_requested() {
    return IO_RB2_GetValue();
}

uint8_t is_drive_requested() {
    return IO_RA0_GetValue();
}

// Pedals
// On the breadboard, the range of values for the potentiometer is 0 to 4095

#define PEDAL_MAX 4095

// There is some noise when reading from the brake pedal
// So give some room for error when driver presses on brake
#define BRAKE_ERROR_TOLERANCE 20


uint16_t throttle1 = 0;
uint16_t throttle2 = 0;
uint16_t throttle1_max= 0;
uint16_t throttle2_max = 0;
uint16_t throttle1_min = 0;
uint16_t throttle2_min = 0;
uint16_t throttle_avg = 0;
uint16_t throttle_range = 0;

uint16_t brake1 = 0;
uint16_t brake2 = 0;
uint16_t brake1_max= 0;
uint16_t brake2_max= 0;
uint16_t brake1_min = 0;
uint16_t brake2_min = 0;
uint16_t brake_avg = 0;
uint16_t brake_range = 0;


// TODO: replace below functions with single function to update all relevant variables
// as of now we don't really have to add more pin reads and potentiometers but it should be ready to implement when needed
// See update_ADC_SAR() in pedal node

uint16_t get_brake_pedal_value() {
    return ADCC_GetSingleConversion(channel_ANB0);
}

uint16_t get_gas_pedal_value() {
    return ADCC_GetSingleConversion(channel_ANB1);
}

// How long to wait for pre-charging to finish before timing out
#define MAX_CONSERVATION_SECS 4
// Keeps track of timer waiting for pre-charging
unsigned int conservative_timer_ms = 0;
// Delay between checking pre-charging state
#define AWAIT_PRECHARGING_DELAY_MS 100

// High voltage state variables
#define DRIVE_REQ_DELAY_MS 1000

// Initial FSM state
state_t state = LV;
error_t error = NONE;

const char* STATE_NAMES[] = {"LV", "PRECHARGING", "HV_ENABLED", "DRIVE", "FAULT"};
// TODO: add sensor discrepancy error
const char* ERROR_NAMES[] = {"NONE", "DRIVE_REQUEST_FROM_LV", "CONSERVATIVE_TIMER_MAXED", "BRAKE_NOT_PRESSED", "HV_DISABLED_WHILE_DRIVE"};

void change_state(const state_t new_state) {
    // Handle edge cases
    if (state == FAULT && new_state != FAULT) {
        // Reset the error cause when exiting fault state
        error = NONE;
    }
        
    // Print state transition
    printf("%s -> %s\r\n", STATE_NAMES[state], STATE_NAMES[new_state]);
    
    state = new_state;
}

void report_fault(error_t _error) {
    change_state(FAULT);
    
    printf("Error: %s\r\n", ERROR_NAMES[_error]);
    
    // Cause of error
    error = _error;
}

// TODO: write function to process and send pedal and brake data over CAN
// see CY_ISR(isr_CAN_Handler) in pedal node

// TODO: write function to check differential between the throttle sensors and brake sensors
// returns if the sensor discrepancy is > 3%
// see check_differential() in pedal node

// TODO: write functions to save and load calibration data
// see EEPROM functions in pedal node
// probably dont need this if we are always recalibrating on startup/lv

void main() {
    // Reset PIC18
    SYSTEM_Initialize();

    // Set up ADCC for reading analog signals
    ADCC_DischargeSampleCapacitor();

    // Only for debugging. Use this to test the controls on the breadboard
    #if 0
    while (1)
    {
        printf("Pot 0: %d\r\n", get_brake_pedal_value());
        printf("Pot 1: %d\r\n", get_gas_pedal_value());
        printf("Switch 2: %d\r\n", is_hv_requested());
        printf("Drive switch: %d\r\n\n", is_drive_requested());
        __delay_ms(1000);
    }
    #endif
    
    printf("Starting in %s state", STATE_NAMES[state]);
    
    // TODO: set throttle and brake mins/maxs to opposite of range
    // see calibrating state in main() in pedal node
    
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
                
                // TODO: add calibration
                // update values, check if > max or < min and update accordingly
                // see calibrating helper state in main() in pedal node
                
                break;
            case PRECHARGING:
                if (conservative_timer_ms >= MAX_CONSERVATION_SECS * 1000) {
                    // Pre-charging took too long
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
                
                // TODO: check using clock cycles or system clock instead
                // Check if pre-charge is finished for every delay
                __delay_ms(AWAIT_PRECHARGING_DELAY_MS);
                conservative_timer_ms += AWAIT_PRECHARGING_DELAY_MS;
                break;
            case HV_ENABLED:
                if (!is_hv_requested()) {
                    // Driver flipped off HV switch
                    // TODO: or capacitor voltage went under threshold
                    change_state(LV);
                    break;
                }
                
                if (is_drive_requested()) {
                    // Driver flipped on drive switch
                    // Need to press on pedal at the same time to go to drive
                    if (get_brake_pedal_value() >= PEDAL_MAX - BRAKE_ERROR_TOLERANCE) {
                        
                        change_state(DRIVE);                        
                    } else {
                        // Driver didn't press pedal
                        report_fault(BRAKE_NOT_PRESSED);
                    }
                }
                break;
            case DRIVE:
                if (!is_drive_requested()) {
                    // Drive switch was flipped off
                    // Revert to HV
                    change_state(HV_ENABLED);
                   break;
                }

                if (!is_hv_requested()) {
                    // HV switched flipped off, so can't drive
                    report_fault(HV_DISABLED_WHILE_DRIVING);
                }
                
                // TODO: add check for sensor discrepancy, send fault if so
                
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
                    case HV_DISABLED_WHILE_DRIVING:
                        if (!is_drive_requested()) {
                            // Ask driver to flip off drive switch to properly go back to LV
                            change_state(LV);
                        }
                        break;
                        
                    // TODO: add sensor discrepancy fault
                    // unresolvable?
                }
                break;
        }
    }
}
