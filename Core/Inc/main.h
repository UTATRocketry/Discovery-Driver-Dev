/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MCU_GSEPWR_EN_Pin GPIO_PIN_8
#define MCU_GSEPWR_EN_GPIO_Port GPIOF
#define MCU_BATT_EN_Pin GPIO_PIN_9
#define MCU_BATT_EN_GPIO_Port GPIOF
#define MCU_OSC_IN_Pin GPIO_PIN_0
#define MCU_OSC_IN_GPIO_Port GPIOH
#define V8V4_IN_Pin GPIO_PIN_0
#define V8V4_IN_GPIO_Port GPIOC
#define V24_IN_Pin GPIO_PIN_1
#define V24_IN_GPIO_Port GPIOC
#define BATT_IN_Pin GPIO_PIN_2
#define BATT_IN_GPIO_Port GPIOC
#define MAIN_IN_Pin GPIO_PIN_3
#define MAIN_IN_GPIO_Port GPIOC
#define VOUT_ISENSE_8V4_Pin GPIO_PIN_10
#define VOUT_ISENSE_8V4_GPIO_Port GPIOE
#define VOUT_ISENSE_24V_Pin GPIO_PIN_11
#define VOUT_ISENSE_24V_GPIO_Port GPIOE
#define VOUT_ISENSE_MAIN_Pin GPIO_PIN_12
#define VOUT_ISENSE_MAIN_GPIO_Port GPIOE
#define VCP_RX_Pin GPIO_PIN_10
#define VCP_RX_GPIO_Port GPIOB
#define VCP_TX_Pin GPIO_PIN_11
#define VCP_TX_GPIO_Port GPIOB
#define MCU_LED_1_Pin GPIO_PIN_13
#define MCU_LED_1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
