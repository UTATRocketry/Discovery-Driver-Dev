/**
 ******************************************************************************
 * @file    lis3mdl_driver.c
 * @author  Amelia Ellis
 * @brief   RunCam  driver header file
 ******************************************************************************
*/
// Put this under USER CODE BEGIN Includes
#include "../../Drivers/RunCam_Driver/runcam_driver.h"
#include <stdio.h>
#include <stdarg.h>

// Put this under USER CODE BEGIN 0
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

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  debug_printf("\nStarting RunCam Driver Test\n");

  // Initialize RunCam
  runcam_init(&huart1);
  if (runcam_get_status() == RUNCAM_OK) {
	  debug_printf("\nRunCam Initialized Successfully!\n");
  } else {
	  debug_printf("\nRunCam Initialization Error:\n %s\n", runcam_status_to_string(runcam_get_status()));
  }


  // Example usage: Read device information
  uint8_t device_info[5];
  runcam_get_device_info(device_info);
  if (runcam_get_status() == RUNCAM_OK) {
	  debug_printf("\nRunCam Device Info Retrieved Successfully\n");
  } else {
	  debug_printf("\nFailed to retrieve device info, Error:\n %s\n", runcam_status_to_string(runcam_get_status()));
  }

  // Example usage: Read all settings
  runcam_setting_t settings[NUM_RUNCAM_SETTINGS];
  runcam_get_all_settings(settings);
  if (runcam_get_status() == RUNCAM_OK) {
      debug_printf("\n");
      for (uint8_t i = 0; i < NUM_RUNCAM_SETTINGS; i++) {
    	  debug_printf("%s: %s\n", settings[i].setting_id, settings[i].value);
      }
  } else {
	  debug_printf("\nFailed to read settings, Error:\n %s\n", runcam_status_to_string(runcam_get_status()));
  }


  // Example usage: Read a specific setting (e.g., resolution)
  uint8_t setting_response[10];
  runcam_get_setting(RUNCAM_SETTING_RESOLUTION, setting_response, 0);
  if (runcam_get_status() == RUNCAM_OK) {
	  debug_printf("\nResolution: %s\n", &setting_response[4]);
  } else {
	  debug_printf("\nFailed to read resolution, Error:\n %s\n", runcam_status_to_string(runcam_get_status()));
  }


  // Example usage: Change resolution setting (e.g., set to 1080P@60fps)
  uint8_t new_resolution[] = "1080P60";
  runcam_write_setting(RUNCAM_SETTING_RESOLUTION, new_resolution, strlen((char *)new_resolution));
  if (runcam_get_status() == RUNCAM_OK) {
	  debug_printf("\nResolution updated successfully\n");
  } else {
	  debug_printf("\nFailed to update resolution, Error:\n %s\n", runcam_status_to_string(runcam_get_status()));
  }

  // Example usage: Start recording
  runcam_start_recording();
  if (runcam_get_status() == RUNCAM_OK) {
	  debug_printf("\nRunCam started recording\n");
	  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
  } else {
	  debug_printf("\nFailed to start recording, Error:\n %s\n", runcam_status_to_string(runcam_get_status()));
  }

  HAL_Delay(10000);  // Record for 10 seconds
  runcam_stop_recording();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

