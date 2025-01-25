/*
 * RCSplit_Driver.c
 *
 *  Created on: Jan 21, 2025
 *      Author: Amelia Ellis
 */


#include "RCSplit_Driver.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

UART_HandleTypeDef* RC_huart3;

void debug_printf(const char *fmt, ...) {
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  uint16_t i = 0;
  while(buffer[i] != '\0') {
    ITM_SendChar(buffer[i]);
    i++;
  }

}

char* runcam_status_to_string(HAL_StatusTypeDef* status) {
	char* str;
	if (*status == 0x00) {
		str = "HAL_OK";
	}
	if (*status == 0x01) {
		str = "HAL_ERROR";
	}
	if (*status == 0x02) {
		str = "HAL_BUSY";
	}
	if (*status == 0x03) {
		str = "HAL_TIMEOUT";
	}
	return str;
}

// Initialize the Run Cam, returns 0 if successful, 1 if failed
int runcam_init(UART_HandleTypeDef* huart3) {
	RC_huart3 = huart3;
	uint8_t device_info[5];

	int read_success = runcam_get_device_info(&huart3, &device_info);

	if (read_success == 0) {
			debug_printf("\nRunCam Initialized Successfully!\n");

	    } else {
		    debug_printf("\nRunCam Initialization Error\n");
	    }
}

// Checks if the Run Cam is connected, returns 0 if successful, 1 if failed
int runcam_check_connection(UART_HandleTypeDef* huart3) {
	RC_huart3 = huart3;
}

// Read device information, returns 0 if successful, 1 if failed
int runcam_get_device_info(UART_HandleTypeDef* huart3, uint8_t* p_device_info){
	RC_huart3 = huart3;

	HAL_StatusTypeDef status;

	uint8_t request = 0xCC0000; // idk about byte 3/3

	status = HAL_UART_Transmit_IT(huart3, &request, 3); // size is in bytes, right?

	if (status == HAL_OK) {
			debug_printf("\nRunCam Device Request Sent Successfully\n");
		} else {
			debug_printf("\nFailed to transmit request, Error:\n %s\n", runcam_status_to_string(&status));
			return 1;
		}

	status = HAL_UART_Receive_IT(huart3, p_device_info, 5);

	if (status == HAL_OK) {
		debug_printf("\nRunCam Device Info Retrieved Successfully\n");
		return 0;
	} else {
		debug_printf("\nFailed to retrieve device info, Error:\n %s\n", runcam_status_to_string(&status));
		return 1;
	}

}
