#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

#include "stm32l4xx_hal.h"
#include <string.h>
#include "main.h"

/*
from datasheet at https://www.st.com/en/microcontrollers-microprocessors/stm32l4r5zi.html page 17:
TIM3 is connected to APB1 bus --> 120 MHz from the clock configuration diagram under APB1 timer clocks
*/
// #define F_CLK                         120000000 // 120 MHz 
// F_PWM = 50 Hz
#define PSC_50                        59999 // can now get prescaler PSC = F_CLK/((ARR+1)F_PWM) - 1
#define ARR_50                        39
// F_PWM = 330 Hz
#define PSC_330                       60605 // not sure how to change these in code though (without using the .ioc file)
#define ARR_330                       5        

// parameters
// #define DUTY_CYCLE_FREQ               0.02 // 50 Hz (do not delete; you do use this)
#define MIN_PULSE_WIDTH               0.0005 // s = 500 microseconds --> 0 degreee position
#define MAX_PULSE_WIDTH               0.0025 // s = 2500 microseconds --> 180 degree position
#define NEUTRAL_PULSE_WIDTH           0.0015 // s = 1500 microseconds --> 90 degree position

// value btw 0 and 1 = propotion of period used by duty cycle
// #define DUTY_CYCLE(PULSE_WIDTH)       PULSE_WIDTH*DUTY_CYCLE_FREQ


uint8_t init_servo_channels(uint32_t *frequency, TIM_HandleTypeDef *p_htim, uint8_t CH1, uint8_t CH2, uint8_t CH3, uint8_t CH4);
uint8_t servo_driver_set_pw(double *pulse_witdth, TIM_HandleTypeDef *p_htim, uint8_t CH);

#endif