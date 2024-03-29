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
    BRAKE_NOT_PRESSED,
    HV_DISABLED_WHILE_DRIVING,
    SENSOR_DISCREPANCY,
    BRAKE_IMPLAUSIBLE
} error_t;

// Controls

// Switches
// 0 means off, 1 means on

uint8_t is_hv_requested() {
    return IO_RB2_GetValue();
}

uint8_t is_drive_requested() {
    return IO_RB7_GetValue();
}

// Pedals
// On the breadboard, the range of values for the potentiometer is 0 to 4095

#define PEDAL_MAX 4095

// There is some noise when reading from the brake pedal
// So give some room for error when driver presses on brake
#define BRAKE_ERROR_TOLERANCE 50


uint16_t throttle1 = 0;
uint16_t throttle2 = 0;
uint16_t throttle1_max = 0;
uint16_t throttle1_min = 0x7FFF;
uint16_t throttle2_max = 0;
uint16_t throttle2_min = 0x7FFF;
uint16_t throttle_range = 0; // set after max and min values are calibrated
uint16_t per_throttle1 = 0;
uint16_t per_throttle2 = 0;

uint16_t brake = 0;
uint16_t brake_max = 0;
uint16_t brake_min = 0;
uint16_t brake_range = 0;

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

const char* STATE_NAMES[] = {
    "LV", 
    "PRECHARGING", 
    "HV_ENABLED", 
    "DRIVE", 
    "FAULT"
};
const char* ERROR_NAMES[] = {
    "NONE", 
    "DRIVE_REQUEST_FROM_LV", 
    "CONSERVATIVE_TIMER_MAXED", 
    "BRAKE_NOT_PRESSED", 
    "HV_DISABLED_WHILE_DRIVE",
    "SENSOR_DISCREPANCY",
    "BRAKE_IMPLAUSIBLE"
};

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


bool start_calibration = true;

void run_calibration() {
    if (start_calibration) {
        // set up values at start of calibration
        throttle1_max = 0;
        throttle1_min = 0x7FFF;
        throttle2_max = 0;
        throttle2_min = 0x7FFF;
        brake_max = 0;
        brake_min = 0x7FFF;
        start_calibration = false;
    }
    else {
        throttle1 = ADCC_GetSingleConversion(channel_ANB0);
        throttle2 = ADCC_GetSingleConversion(channel_ANB1);
        brake = ADCC_GetSingleConversion(channel_ANB5);

        if (throttle1 > throttle1_max) {
            throttle1_max = throttle1;
        }
        if (throttle1 < throttle1_min) {
            throttle1_min = throttle1;
        }
        if (throttle2 > throttle2_max) {
            throttle2_max = throttle2;
        }
        if (throttle2 < throttle2_min) {
            throttle2_min = throttle2;    
        }

        if (brake > brake_max) {
            brake_max = brake;
        }
        if (brake < brake_min) {
            brake_min = brake;
        }    

         printf("throttle1: %d\r\n", throttle1);
         printf("throttle1_max: %d\r\n", throttle1_max);
         printf("throttle1_min: %d\r\n", throttle1_min); 
         printf("throttle2: %d\r\n", throttle2);         
         printf("throttle2_max: %d\r\n", throttle2_max);
         printf("throttle2_min: %d\r\n", throttle2_min);
         printf("brake: %d\r\n", brake);
         printf("brake_max: %d\r\n", brake_max);
         printf("brake_min: %d\r\n", brake_min);
    }
}

// check differential between the throttle sensors
// returns true only if the sensor discrepancy is > 10%
// Note: after verifying there's no discrepancy, can use either sensor(1 or 2) for remaining checks
bool has_discrepancy() {
    // check throttle
    
    // calculate percentage of throttle 1
    uint16_t temp_throttle1 = throttle1;
    if (temp_throttle1 > throttle1_max) {
        temp_throttle1 = throttle1_max;
    } else if (temp_throttle1 < throttle1_min) {
        temp_throttle1 = throttle1_min;
    }
    per_throttle1 = (uint16_t)(((double)(temp_throttle1-throttle1_min) / (throttle1_max - throttle1_min)) * 100);
    
    // calculate percentage of throttle 2
    uint16_t temp_throttle2 = throttle2;
    if (temp_throttle2 > throttle2_max) {
        temp_throttle2 = throttle2_max;
    } else if( temp_throttle2 < throttle2_min) {
        temp_throttle2 = throttle2_min;
    }
    per_throttle2 = (uint16_t)(((double)(temp_throttle2-throttle2_min) / (throttle2_max - throttle2_min)) * 100);
    
    return abs((int)per_throttle1 - (int)per_throttle2) > 10;
}

// check for soft BSPD
// see EV.5.7 of FSAE 2022 rulebook
bool brake_implausible() {
    uint16_t temp_throttle = throttle1; 
    
    // subtract dead zone 15%
    uint16_t temp_brake = brake - ((throttle_range)/6);
    if (temp_brake > brake_max){
        temp_brake = brake_max;
    }
    if (temp_brake < brake_min){
        temp_brake = brake_min;
    }
    temp_brake = (uint16_t)((temp_brake-brake_min)*100.0 / throttle_range);
    
    
    if (state == FAULT && error == BRAKE_IMPLAUSIBLE) {
        // once brake implausibility detected, can only revert to normal if throttle unapplied
        return !(temp_throttle < throttle_range * 0.25);
    }
    else {
        // if both brake and throttle applied, brake implausible
        return (temp_brake > 0 && temp_throttle > throttle_range * 0.25);
    }
    
}

// storage variables used to return to previous state when discrepancy is resolved
state_t temp_state = LV; // state before sensor discrepancy error
error_t temp_error = NONE; // error state before sensor discrepancy error (only used when going from one fault to discrepancy fault)

void update_sensor_vals() {
    throttle1 = ADCC_GetSingleConversion(channel_ANB0);
    throttle2 = ADCC_GetSingleConversion(channel_ANB1); 
    brake = ADCC_GetSingleConversion(channel_ANB5);
    
     printf("State: %s\r\n", STATE_NAMES[state]);
     printf("Throttle 1: %d\r\n", throttle1);
     printf("Throttle 2: %d\r\n", throttle2);
     printf("Brake: %d\r\n", brake);

   if (error != SENSOR_DISCREPANCY && has_discrepancy() ) {
       temp_state = state;
       temp_error = error;
       report_fault(SENSOR_DISCREPANCY);
   }
}


// TODO: write function to process and send pedal and brake data over CAN
// see CY_ISR(isr_CAN_Handler) in pedal node

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
        update_sensor_vals();
        printf("Throttle 1: %d\r\n", throttle1);
        printf("Throttle 2: %d\r\n", throttle2);
        printf("Brake : %d\r\n", brake);
        printf("HV switch: %d\r\n", is_hv_requested());
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
        
        printf("-----------------------\r\n");
        
        switch (state) {
            case LV:
                run_calibration();
                
                if (is_drive_requested()) {
                    // Drive switch should not be enabled during LV
                    report_fault(DRIVE_REQUEST_FROM_LV);
                    break;
                }

                if (is_hv_requested()) {
                    // HV switch was flipped
                    
                    // Set throttle and brake range since calibration is done
                    throttle_range = throttle1_max - throttle1_min;
                    brake_range = brake_max - brake_min; // idk where this is even used
                    
                    // Start charging the car to high voltage state
                    change_state(PRECHARGING);
                } 
                
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
                update_sensor_vals();

                if (!is_hv_requested()) {
                    // Driver flipped off HV switch
                    // TODO: or capacitor voltage went under threshold
                    change_state(LV);
                    break;
                }
                
                if (is_drive_requested()) {
                    // Driver flipped on drive switch
                    // Need to press on pedal at the same time to go to drive
                    if (brake >= PEDAL_MAX - BRAKE_ERROR_TOLERANCE) {

                        change_state(DRIVE);                        
                    } else {
                        // Driver didn't press pedal
                        report_fault(BRAKE_NOT_PRESSED);
                    }
                }
                
                break;
            case DRIVE:
                update_sensor_vals();

                if (!is_drive_requested()) {
                    // Drive switch was flipped off
                    // Revert to HV
                    change_state(HV_ENABLED);
                   break;
                }

                if (!is_hv_requested()) {
                    // HV switched flipped off, so can't drive
                    report_fault(HV_DISABLED_WHILE_DRIVING);
                    break;
                }

                if (brake_implausible()) {
                    report_fault(BRAKE_IMPLAUSIBLE);
                }
                
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
                        update_sensor_vals();

                        if (!is_drive_requested()) {
                            // Ask driver to flip off drive switch to properly go back to LV
                            change_state(LV);
                        }
                        break;
                    case SENSOR_DISCREPANCY:
                        update_sensor_vals();
                        
                        // TODO: stop power to motors if discrepancy persists for >100ms
                        // see rule T.4.2.5 in FSAE 2022 rulebook

                        if (!has_discrepancy()) {
                            // if discrepancy resolved, change back to previous state
                            if (temp_state == FAULT) {
                                report_fault(temp_error);
                            } else {
                                change_state(temp_state);
                            }
                        }
                        break;
                    case BRAKE_IMPLAUSIBLE:
                        update_sensor_vals();

                        if (!brake_implausible()) {
                            change_state(DRIVE);
                        }
                }
                break;
        }
        
    }
}
