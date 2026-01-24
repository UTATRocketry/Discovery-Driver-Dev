#include "main.h"
#include "NEOM9N.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);

unsigned char GPSData[2048] = {0};

int format_float(float value, int decimals, char* buffer) {
    int int_part = (int)value;
    float frac_part = value - (float)int_part;
    if (frac_part < 0) frac_part = -frac_part;
    
    if (value < 0) {
        buffer[0] = '-';
        buffer++;
        int_part = -int_part;
    }
    
    // Convert integer part
    int len = 0;
    if (int_part == 0) {
        buffer[len++] = '0';
    } else {
        char temp[20];
        int i = 0;
        while (int_part > 0) {
            temp[i++] = '0' + (int_part % 10);
            int_part /= 10;
        }
        for (int j = i - 1; j >= 0; j--) {
            buffer[len++] = temp[j];
        }
    }
    
    if (decimals > 0) {
        buffer[len++] = '.';
        for (int i = 0; i < decimals; i++) {
            frac_part *= 10;
            int digit = (int)frac_part;
            buffer[len++] = '0' + digit;
            frac_part -= (float)digit;
        }
    }
    
    buffer[len] = '\0';
    return len + (value < 0 ? 1 : 0);
}

int main(void)
{
	int status = 0;
	float longitude = 0, latitude = 0, time = 0, speed = 0, altitude = 0;
	char latHem = 0, lonHem = 0;

	unsigned char breakline[10] = "\n\r\n\r";
	unsigned char tx_buff[100];

	HAL_Init();
	SystemClock_Config();
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_USART2_UART_Init();
	MX_USART3_UART_Init();
	MX_USB_OTG_FS_PCD_Init();

	sprintf((char*)tx_buff, "GPS Driver Starting...\n\r");
  HAL_UART_Transmit(&hlpuart1, tx_buff, strlen((char*)tx_buff), 2000);
  HAL_Delay(100);
  
  status = NEOM9N_init(&huart2);

  if (status != HAL_OK) {
	  sprintf((char*)tx_buff, "GPS Init Failed!\n\r");
	  HAL_UART_Transmit(&hlpuart1, tx_buff, strlen((char*)tx_buff), 2000);
  } else {
	  sprintf((char*)tx_buff, "GPS Init Success! Waiting for data...\n\r");
	  HAL_UART_Transmit(&hlpuart1, tx_buff, strlen((char*)tx_buff), 2000);
  }

	uint32_t loop_count = 0;
  while (1)
  {
	  loop_count++;
	  
	  if (loop_count % 1000 == 0) {
		  uint32_t int_count = 0, byte_count = 0;
		  NEOM9N_getDiagnostics(&int_count, &byte_count);
		  sprintf((char*)tx_buff, "Heartbeat: Loop %lu, GPS Ready: %d, Interrupts: %lu, Bytes: %lu\n\r", 
		          (unsigned long)loop_count, NEOM9N_isDataReady(), 
		          (unsigned long)int_count, (unsigned long)byte_count);
		  HAL_UART_Transmit(&hlpuart1, tx_buff, strlen((char*)tx_buff), 2000);
	  }
	  
	  if (NEOM9N_isDataReady()) {
		  if (NEOM9N_getData(GPSData) == HAL_OK) {
			  HAL_UART_Transmit(&hlpuart1, GPSData, strlen((char*)GPSData), 2000);
			  HAL_UART_Transmit(&hlpuart1, breakline, strlen((char*)breakline), 2000);

			  // GPS DATA PARSING: Extract time from NMEA data
			  status = NEOM9N_getTime(&time, GPSData, sizeof(GPSData));
			  if (status == 0) {
				  int hour = (int)(time / 10000);
				  int minute = (int)((time - hour * 10000) / 100);
				  float second = time - hour * 10000 - minute * 100;
				  int sec_int = (int)second;
				  int sec_frac = (int)((second - sec_int) * 100);
				  sprintf((char*)tx_buff, "UTC Time: %02d:%02d:%02d.%02d\n\r", hour, minute, sec_int, sec_frac);
			  } else {
				  sprintf((char*)tx_buff, "Could not parse time from GPS data.\n\r");
			  }
			  HAL_UART_Transmit(&hlpuart1, tx_buff, strlen((char*)tx_buff), 2000);

			  // GPS DATA PARSING: Extract position from NMEA data
			  if (NEOM9N_getPosition(&latitude, &latHem, &longitude, &lonHem, GPSData,
			                          sizeof(GPSData)) == 0) {
				  int lat_deg = (int)latitude;
				  int lat_min_frac = (int)((latitude - lat_deg) * 6000000);
				  int lon_deg = (int)longitude;
				  int lon_min_frac = (int)((longitude - lon_deg) * 6000000);
				  sprintf((char*)tx_buff, "Latitude: %d.%06d deg %c\n\rLongitude: %d.%06d deg %c\n\r", 
				          lat_deg, lat_min_frac, latHem, lon_deg, lon_min_frac, lonHem);
			  } else {
				  sprintf((char*)tx_buff, "Could not parse position from GPS data.\n\r");
			  }
			  HAL_UART_Transmit(&hlpuart1, tx_buff, strlen((char*)tx_buff), 2000);

			  // GPS DATA PARSING: Extract speed from NMEA data
			  if (NEOM9N_getSpeed(&speed, GPSData) == 0) {
				  int speed_int = (int)speed;
				  int speed_frac = (int)((speed - speed_int) * 100);
				  sprintf((char*)tx_buff, "Speed: %d.%02d knots\n\r", speed_int, speed_frac);
			  } else {
				  sprintf((char*)tx_buff, "Could not parse speed from GPS data.\n\r");
			  }
			  HAL_UART_Transmit(&hlpuart1, tx_buff, strlen((char*)tx_buff), 2000);

			  // GPS DATA PARSING: Extract altitude from NMEA data
			  if (NEOM9N_getAltitude(&altitude, GPSData) == 0) {
				  int alt_int = (int)altitude;
				  int alt_frac = (int)((altitude - alt_int) * 100);
				  sprintf((char*)tx_buff, "Altitude: %d.%02d m\n\r", alt_int, alt_frac);
			  } else {
				  sprintf((char*)tx_buff, "Could not parse altitude from GPS data.\n\r");
			  }
			  HAL_UART_Transmit(&hlpuart1, tx_buff, strlen((char*)tx_buff), 2000);
		  }
	  }
	  
	  HAL_Delay(10);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 30;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_LPUART1_UART_Init(void)
{
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 38400;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USB_OTG_FS_PCD_Init(void)
{
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.battery_charging_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOB, LD3_Pin|LD2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
