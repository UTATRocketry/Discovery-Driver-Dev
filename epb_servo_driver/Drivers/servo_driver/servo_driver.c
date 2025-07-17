#include "servo_driver.h"

// TIM_HandleTypeDef htim4;

static uint16_t arr;
static uint16_t psc;
static uint32_t pulse_width;
static uint32_t freq;

/*
initialise the servo angle to neutral
Return uint8_t init_fail
0: initialisation successful
1: initialisation failed (invalid frequency)
2: initialisation failed (invalid channel number -- must be a value between 1 and 4)
*/
uint8_t init_servo_channels(uint32_t *frequency, TIM_HandleTypeDef *p_htim, uint8_t CH1, uint8_t CH2, uint8_t CH3, uint8_t CH4) {
    uint8_t channels[4] = {CH1, CH2, CH3, CH4};
    
    // start the pwm signal for all the above timer 3 channels
    for (int i = 0; i < 4; i++) {
        if (channels[i] == 1) {HAL_TIM_PWM_Start(p_htim, TIM_CHANNEL_1);}
        else if (channels[i] == 2) {HAL_TIM_PWM_Start(p_htim, TIM_CHANNEL_2);}
        else if (channels[i] == 3) {HAL_TIM_PWM_Start(p_htim, TIM_CHANNEL_3);}
        else if (channels[i] == 4) {HAL_TIM_PWM_Start(p_htim, TIM_CHANNEL_4);}
        else {return 2;}
    }
    
    if (*frequency == 50) {
        arr = ARR_50;
        psc = PSC_50;
    } else if (*frequency == 330) {
        arr = ARR_330;
        psc = PSC_330;
    } else {
        return 1;
    }
    
    // store initialisation frequency for later
    freq = *frequency;
    
    // initialise to the neutral pulse width
    pulse_width = NEUTRAL_PULSE_WIDTH;
    for (int i = 0; i < 4; i++) {
        servo_driver_set_pw(&pulse_width, p_htim, channels[i]);
    }

    return 0;
}

// pulse width (=[0.001s, 0.002s]-->[0 degrees, 180 degrees]) is converted to number of pulses
// (ARR number of pulses per clock cycle (50 Hz or 330 Hz) --> each pulse is 1/(freq)/(arr+1) seconds) (https://deepbluembedded.com/stm32-pwm-example-timer-pwm-mode-tutorial/)
static uint32_t servo_driver_pulse_width_to_num(double *width) {
    double f = (double)freq;
    double a = (double)(arr+1);
    uint32_t pulse = (uint32_t) ((*width)*f*a); // formula from same site as above and https://community.st.com/t5/stm32-mcus-products/stm32-pwm-duty-cycle-setting/td-p/152105
    return pulse;
}

/*
set the servo pulse width
return uint8_t error
    0: no error, servo pulse width set successfully to the correct channel
    1: invalid pulse width
    2: invalid channel
*/
uint8_t servo_driver_set_pw(double* width, TIM_HandleTypeDef *p_htim, uint8_t CH) {
    TIM_HandleTypeDef htim = *p_htim;
    uint8_t pulses = servo_driver_pulse_width_to_num(width);
    if (pulses == 0) {
        return 1;
    } 
    //CCRx --> changes the pwm signal of the x-th channel for the timer htim
    if (CH == 1) {htim.Instance->CCR1 = pulses;}
    else if (CH == 2) {htim.Instance->CCR2 = pulses;}
    else if (CH == 3) {htim.Instance->CCR3 = pulses;}
    else if (CH == 4) {htim.Instance->CCR4 = pulses;}
    else {return 2;}
       
    return 0;
}