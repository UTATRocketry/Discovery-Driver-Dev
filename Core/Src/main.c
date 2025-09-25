/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "heatshrink_config.h"
#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"
#include "golay24.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//int UART_printf(const char *fmt, ...) {
//	char buf[UART_BUF];
//	va_list args;
//	int len;
//
//	va_start(args, fmt);
//	len = vsnprintf(buf, sizeof(buf), fmt, args);
//	va_end(args);
//
//	HAL_UART_Transmit(&huart2, (uint8_t*) buf, len, 1000);
//
//	return len;
//}
void blink_byte(uint8_t byte, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
	for (int i = 7; i >= 0; i--) {
		HAL_GPIO_WritePin(GPIOx, GPIO_Pin, (byte >> i) & 1);
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
		HAL_Delay(500);  // 200ms per bit
	}
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
	HAL_Delay(500);  // Pause between bytes
}

void blink_buffer(uint8_t *buf, size_t len, GPIO_TypeDef *GPIOx,
		uint16_t GPIO_Pin) {
	for (size_t i = 0; i < len; i++) {
		blink_byte(buf[i], GPIOx, GPIO_Pin);
	}
}

/* USER CODE END 0 */

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
  /* USER CODE BEGIN 2 */
	heatshrink_encoder encoder;
	heatshrink_decoder decoder;

	uint8_t in_buf[16] = "hello";
	uint8_t comp_buf[16];
	uint8_t golay_encoded_buf[48];  // Golay encoded buffer (3 bytes per input byte)
	uint8_t golay_decoded_buf[16];  // Golay decoded buffer
	uint8_t out_buf[32];  // Decompressed output

	size_t in_size = strlen((char*) in_buf);
	size_t sunk = 0, polled = 0, total_polled = 0;
	size_t sunk_d = 0, polled_d = 0, total_out = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		heatshrink_encoder_reset(&encoder);

		blink_buffer(in_buf, in_size, GPIOB, GPIO_PIN_13); // Blink input on LED3

		/* ENCODING */
		// sinking data
		while (sunk < in_size) {
			size_t s = 0;
			heatshrink_encoder_sink(&encoder, in_buf + sunk, in_size - sunk,
					&s);
			sunk += s;

			// polling output
			while (1) {
				size_t p = 0;
				HSE_poll_res pres = heatshrink_encoder_poll(&encoder,
						comp_buf + total_polled,
						sizeof(comp_buf) - total_polled, &p);
				total_polled += p;
				if (pres == HSER_POLL_EMPTY)
					break;
			}
		}
		heatshrink_encoder_finish(&encoder);
		// flush again, poll again
		while (1) {
			size_t p = 0;
			HSE_poll_res pres = heatshrink_encoder_poll(&encoder,
					comp_buf + total_polled, sizeof(comp_buf) - total_polled,
					&p);
			total_polled += p;
			if (pres == HSER_POLL_EMPTY)
				break;
		}

		// GOLAY ENCODING - fit one byte per 12-bit chunk
		// Each compressed byte is encoded into a 24-bit Golay codeword (3 bytes)
		size_t golay_encoded_size = 0;
		for (size_t i = 0; i < total_polled; i++) {
			// Take each byte from comp_buf and treat it as 8-bit data
			// Pad to 12 bits and encode with Golay (8 bits fits within 12-bit data field)
			uint32_t data_12bit = (uint32_t)comp_buf[i];  // 8 bits expanded to 12 bits
			uint32_t codeword = golay24_data2code(data_12bit);
			
			// Store the 24-bit codeword as 3 bytes
			golay_encoded_buf[golay_encoded_size++] = (codeword >> 16) & 0xFF;
			golay_encoded_buf[golay_encoded_size++] = (codeword >> 8) & 0xFF;
			golay_encoded_buf[golay_encoded_size++] = codeword & 0xFF;
		}

		// GOLAY DECODING
		// Decode each 24-bit codeword back to 8-bit data
		size_t golay_decoded_size = 0;
		int golay_errors = 0;
		for (size_t i = 0; i < golay_encoded_size; i += 3) {
			// Reconstruct the 24-bit codeword from 3 bytes (big-endian)
			uint32_t codeword = ((uint32_t)golay_encoded_buf[i] << 16) |
								((uint32_t)golay_encoded_buf[i+1] << 8) |
								((uint32_t)golay_encoded_buf[i+2]);
			
			uint32_t decoded_data;
			int result = golay24_code2data(codeword, &decoded_data);
			
			if (result >= 0) {  // Success or corrected errors
				golay_decoded_buf[golay_decoded_size++] = (uint8_t)(decoded_data & 0xFF);
				if (result > 0) {
					golay_errors += result;  // Count corrected errors
				}
			} else {
				// Error correction fail catch uhhh yeah don't want to write anything here tbh
				break;
			}
		}

		// HEATSHRINK DECODE
		// Only proceed if Golay decoding was successful
		if (golay_decoded_size > 0) {
			heatshrink_decoder_reset(&decoder);
			HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);  // LED2 ON

			// Sink Golay decoded input
			while (sunk_d < golay_decoded_size) {
				size_t s = 0;
				heatshrink_decoder_sink(&decoder, golay_decoded_buf + sunk_d,
						golay_decoded_size - sunk_d, &s);
				sunk_d += s;

				heatshrink_decoder_poll(&decoder, out_buf + total_out,
						sizeof(out_buf) - total_out, &polled_d);
				total_out += polled_d;
			}

			heatshrink_decoder_finish(&decoder);
			// Final flush
			while (1) {
				size_t p = 0;
				HSD_poll_res dres = heatshrink_decoder_poll(&decoder,
						out_buf + total_out, sizeof(out_buf) - total_out, &p);
				total_out += p;
				if (dres == HSDR_POLL_EMPTY)
					break;
			}

			blink_buffer(out_buf, total_out, GPIOB, GPIO_PIN_14); // Blink output on LED3
		}

		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);  // LED2 OFF
		sunk = 0, polled = 0, total_polled = 0;
		sunk_d = 0, polled_d = 0, total_out = 0;
		memset(comp_buf, 0, sizeof(comp_buf));
		memset(golay_encoded_buf, 0, sizeof(golay_encoded_buf));
		memset(golay_decoded_buf, 0, sizeof(golay_decoded_buf));
		memset(out_buf, 0, sizeof(out_buf));
		HAL_Delay(2000);
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED3_Pin|LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED3_Pin LED2_Pin */
  GPIO_InitStruct.Pin = LED3_Pin|LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : LED1_Pin */
  GPIO_InitStruct.Pin = LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

