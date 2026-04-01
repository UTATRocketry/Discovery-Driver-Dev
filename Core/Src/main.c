/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
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
// #include "NEOM9N.h"
#include <stdio.h>
#include <string.h>

#include "gps.h"
#include "gps_parser.h"
#include "gps_uart.h"
#include "ring_buffer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define GPS_DMA_BUF_LEN 256  // power of 2
#define GPS_RB_LEN 2048      // power of 2
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart5;
DMA_HandleTypeDef hdma_uart5_rx;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */
static GpsFix gpsFix;
static uint32_t lastPrintMs = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_UART5_Init(void);
static HAL_StatusTypeDef gpsSwitchBaud(UART_HandleTypeDef* huart_gps, uint32_t newBaud, uint8_t save);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// We use Receive-to-IDLE interrupt, so we only need HAL_UARTEx_RxEventCallback.
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size) {
    if (huart->Instance == UART5) {
        gpsUartOnRxEvent(&gpsUart, Size);
    }
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
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
    MX_DMA_Init();
    MX_LPUART1_UART_Init();
    MX_USB_OTG_FS_PCD_Init();
    MX_UART5_Init();
    /* USER CODE BEGIN 2 */
    rbInit(&gpsRb, gpsRbStorage, sizeof(gpsRbStorage));
    gpsUartInit(&gpsUart, &huart5, &gpsRb, gpsDmaBuf, sizeof(gpsDmaBuf));
    gpsParserInit(&gpsFix);
    gpsUartStartRx(&gpsUart);

    // comment out if not switching gps baud!
    // save = 0x01 (temp change, i.e. resets back to previously set baud rate with next power cycle).
    // save = 0x07 (permenant change, i.e. must call this function again to change baud rate)
    gpsSwitchBaud(&huart5, (uint32_t)115200, /* save */ (uint8_t)0x01);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        uint8_t tempBuf[128];
        size_t len = gpsUartRead(&gpsUart, tempBuf, sizeof(tempBuf));
        if (len > 0) {
            gpsParserFeed(&gpsFix, tempBuf, len, HAL_GetTick());
        }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSI;
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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief LPUART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_LPUART1_UART_Init(void) {
    /* USER CODE BEGIN LPUART1_Init 0 */

    /* USER CODE END LPUART1_Init 0 */

    /* USER CODE BEGIN LPUART1_Init 1 */

    /* USER CODE END LPUART1_Init 1 */
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
    if (HAL_UART_Init(&hlpuart1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN LPUART1_Init 2 */

    /* USER CODE END LPUART1_Init 2 */
}

/**
 * @brief UART5 Initialization Function
 * @param None
 * @retval None
 */
static void MX_UART5_Init(void) {
    /* USER CODE BEGIN UART5_Init 0 */

    /* USER CODE END UART5_Init 0 */

    /* USER CODE BEGIN UART5_Init 1 */

    /* USER CODE END UART5_Init 1 */
    huart5.Instance = UART5;
    huart5.Init.BaudRate = 38400;
    huart5.Init.WordLength = UART_WORDLENGTH_8B;
    huart5.Init.StopBits = UART_STOPBITS_1;
    huart5.Init.Parity = UART_PARITY_NONE;
    huart5.Init.Mode = UART_MODE_TX_RX;
    huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart5.Init.OverSampling = UART_OVERSAMPLING_16;
    huart5.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart5.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart5.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart5) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&huart5, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&huart5, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&huart5) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN UART5_Init 2 */

    /* USER CODE END UART5_Init 2 */
}

/**
 * @brief USB_OTG_FS Initialization Function
 * @param None
 * @retval None
 */
static void MX_USB_OTG_FS_PCD_Init(void) {
    /* USER CODE BEGIN USB_OTG_FS_Init 0 */

    /* USER CODE END USB_OTG_FS_Init 0 */

    /* USER CODE BEGIN USB_OTG_FS_Init 1 */

    /* USER CODE END USB_OTG_FS_Init 1 */
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
    if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN USB_OTG_FS_Init 2 */

    /* USER CODE END USB_OTG_FS_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {
    /* DMA controller clock enable */
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA1_Channel1_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* USER CODE BEGIN MX_GPIO_Init_1 */

    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    HAL_PWREx_EnableVddIO2();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, LD3_Pin | LD2_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : B1_Pin */
    GPIO_InitStruct.Pin = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : LD3_Pin LD2_Pin */
    GPIO_InitStruct.Pin = LD3_Pin | LD2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pins : STLK_RX_Pin STLK_TX_Pin */
    GPIO_InitStruct.Pin = STLK_RX_Pin | STLK_TX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*Configure GPIO pin : USB_OverCurrent_Pin */
    GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
    GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

    /* USER CODE BEGIN MX_GPIO_Init_2 */

    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// AI (gemini) assisted code //
// UBX 8-bit Fletcher checksum over UBX-CFG-VALSET payload as defied by (u-blox, pg. 48)
static void ubx_checksum(const uint8_t* buffer, uint16_t len, uint8_t* ck_a, uint8_t* ck_b) {
    uint8_t a = 0, b = 0;
    for (uint16_t i = 0; i < len; i++) {
        a = a + buffer[i];
        b = b + a;
    }
    *ck_a = a;
    *ck_b = b;
}

// after sending package to gps to change anything, wait to see if checksum was validated and there was no data corruption
// returns false if checksum failed and packaage rejected
static bool ubx_wait_ack(UART_HandleTypeDef* huart, uint8_t cls, uint8_t id, uint32_t timeout_ms) {
    /* completely AI generated */
    // basically, checking for 1 of 2 possible responses from the gps
    // - UBX-ACK-ACK (accepted), UBX-ACK-NAK (rejected)
    uint32_t start = HAL_GetTick();
    uint8_t b;
    // Simple state machine for UBX-ACK-ACK: B5 62 05 01 02 00 cls id CK_A CK_B
    const uint8_t preamble[] = {0xB5, 0x62, 0x05, 0x01, 0x02, 0x00};
    uint8_t idx = 0;
    uint8_t payload[2];
    uint8_t ck_a, ck_b;

    while ((HAL_GetTick() - start) < timeout_ms) {
        if (HAL_UART_Receive(huart, &b, 1, 10) != HAL_OK) continue;

        // match preamble
        if (idx < sizeof(preamble)) {
            if (b == preamble[idx]) {
                idx++;
            } else {
                idx = (b == preamble[0]) ? 1 : 0;
            }
            continue;
        }

        // read payload (2 bytes)
        payload[0] = b;
        if (HAL_UART_Receive(huart, &payload[1], 1, 10) != HAL_OK) {
            idx = 0;
            continue;
        }

        // read checksum (2 bytes)
        if (HAL_UART_Receive(huart, &ck_a, 1, 10) != HAL_OK) {
            idx = 0;
            continue;
        }
        if (HAL_UART_Receive(huart, &ck_b, 1, 10) != HAL_OK) {
            idx = 0;
            continue;
        }

        // validate checksum of ACK packet (class..payload)
        uint8_t buf[4 + 2];            // 05 01 02 00 + payload
        memcpy(buf, &preamble[2], 4);  // class,id,lenL,lenH
        buf[4] = payload[0];
        buf[5] = payload[1];

        uint8_t calc_a, calc_b;
        ubx_checksum(buf, sizeof(buf), &calc_a, &calc_b);
        if (calc_a != ck_a || calc_b != ck_b) {
            idx = 0;
            continue;
        }  // corrupted ACK

        // check ACK is for our message
        return (payload[0] == cls && payload[1] == id);
    }
    return false;
}

// Send UBX-CFG-VALSET to set CFG-UART1-BAUDRATE (currently set up to be temporary change), then switch STM32 UART5 to match.
static HAL_StatusTypeDef gpsSwitchBaud(UART_HandleTypeDef* huart_gps, uint32_t newBaud, uint8_t save) {
    /* To change the baud rate of the neom9n, must send a complete UBX packet with required details
     * as defied by (u-blox, pg. 89-91) */

    // UBX-CFG-VALSET packet for 115200 Baud on UART1
    uint8_t pkt[20];  // header == 4 bytes, payload == 12 bytes,

    pkt[0] = 0xB5;
    pkt[1] = 0x62;  // header
    pkt[2] = 0x06;
    pkt[3] = 0x8A;  // Class/ID
    pkt[4] = 0x0C;
    pkt[5] = 0x00;  // Length of payload (12 bytes: 4 config + 8 key/val)

    // --- paylod ---
    // to save the baud rate change (and any other change made), set byte 1 => 0x07 (save to ram, bbr, and flash layers)
    // to just test a change but not save, set byte 1 => 0x01 (saves just to ram)
    pkt[6] = 0x00;  // Byte 0: set to 0x00 cause datasheet says so
    pkt[7] = save;  // Byte 1: layers (0x01 = save to RAM only (temp. change for testing purposes))
    pkt[8] = 0x00;
    pkt[9] = 0x00;  // Byte 2-3: 0x00 cause gemini said so

    // Key ID for UART1 on neom9n = 0x40520001 -> little endian: 01 00 52 40
    pkt[10] = 0x01;
    pkt[11] = 0x00;
    pkt[12] = 0x52;
    pkt[13] = 0x40;
    // --- paylod end ---

    // new baud rate in little endian (ai generated)
    pkt[14] = (uint8_t)(newBaud & 0xFF);
    pkt[15] = (uint8_t)((newBaud >> 8) & 0xFF);
    pkt[16] = (uint8_t)((newBaud >> 16) & 0xFF);
    pkt[17] = (uint8_t)((newBaud >> 24) & 0xFF);

    // Checksum (of the payload -> pkt[2] for class + id + payload_length(2) + payload(12). i.e. 16 bytes)
    uint8_t ck_a, ck_b;
    ubx_checksum(&pkt[2], 16, &ck_a, &ck_b);
    pkt[18] = ck_a;
    pkt[19] = ck_b;

    const uint16_t pkt_length = 20;

    // Stop recieving while switching baud rate
    HAL_UART_AbortReceive(huart_gps);
    __HAL_UART_DISABLE_IT(huart_gps, UART_IT_RXNE);
    __HAL_UART_DISABLE_IT(huart_gps, UART_IT_IDLE);

    // Send at CURRENT baud (must match current GPS baud - before changes)
    if (HAL_UART_Transmit(huart_gps, pkt, pkt_length, 200) != HAL_OK) {
        return HAL_ERROR;
    }

    // check that no data corruption occured and the checksum was validated by the gps
    // Wait for confirmation from GPS at OLD baud
    if (!ubx_wait_ack(huart_gps, 0x06, 0x8A, 300)) {
        return HAL_ERROR;  // GPS didn’t accept it, or no ACK seen
    }

    // Give GPS time to switch UART baud
    // neom9n integration module reccomends at least 100 ms
    // pg 26 has good info on why change baud rate and how
    HAL_Delay(200);

    // Switch STM32 UART baud to match
    if (HAL_UART_DeInit(huart_gps) != HAL_OK) {
        return HAL_ERROR;
    }

    huart_gps->Init.BaudRate = newBaud;
    if (HAL_UART_Init(huart_gps) != HAL_OK) {
        return HAL_ERROR;
    }

    // Restart GPS RX-to-idle
    gpsUartStartRx(&gps_uart);

    return HAL_OK;
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
