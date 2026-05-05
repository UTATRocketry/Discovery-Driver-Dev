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
 Notes:
 - can delete all the console_print() calls, only there to help debugging
 - can stop LED2 from toggling by deleting the code from uart callback

 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "gps_config.h"
#include "gps_interface.h"
#include "ring_buffer.h"

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
UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart5;
DMA_HandleTypeDef hdma_uart5_rx;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */

/* GPS data buffers.
 * Sizes are defined in constants.h.
 * DMA_LEN = 128 bytes:  holds one full GGA sentence with margin.
 * GPS_RB_LEN = 256 bytes:  holds about 3 sentences of backlog so the
 *              parser has time to drain the buffer between fixes. */
static RingBuffer gps_rb;
static uint8_t gps_rb_storage[GPS_RB_LEN];
static uint8_t gps_dma_buf[DMA_LEN];

/* Debug counters -- track DMA callback activity.
 * These are volatile because they are written inside an interrupt (the DMA
 * callback) and read in the main loop. Remove once testing is done. */
volatile uint32_t gps_rx_events = 0;     // total number of DMA callbacks fired
volatile uint32_t gps_rx_bytes = 0;      // bytes received in the most recent callback
volatile uint32_t max_gps_rx_bytes = 0;  // track bytes recieved per callback (should be around ~75 with only GGA enabled)

volatile size_t time_delta = 0;        // time between callbacks
volatile uint32_t max_time_delta = 0;  // track time between callback (should be around ~200ms at 5Hz)

volatile uint32_t gps_rx_event_type = 0;
volatile uint32_t gps_idle_events = 0;  // callbacks triggered by UART idle line
volatile uint32_t gps_ht_events = 0;    // callbacks triggered by DMA half-transfer
volatile uint32_t gps_tc_events = 0;    // callbacks triggered by DMA transfer-complete

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */
static void print_gps_fix(void);
static void print_gps_stats(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* -----------------------
 * debug_uart_callback
 * called from HAL_UARTEx_RxEventCallback every time the DMA fires
 * (idle line, half-transfer, or transfer-complete).
 * Tracks event counts and byte counts for debugging.
 *
 * This entire function can be removed once the driver is confirmed
 * working.
 * ----------------------- */
void debug_uart_callback(UART_HandleTypeDef* huart, uint16_t Size) {
    static uint16_t old_pos = 0;
    uint16_t new_bytes = 0;
    static uint32_t last_callback_time = 0;

    /* figure out which type of event triggered this callback */
    HAL_UART_RxEventTypeTypeDef evt = HAL_UARTEx_GetRxEventType(huart);
    if (evt == HAL_UART_RXEVENT_IDLE)
        gps_idle_events++;
    else if (evt == HAL_UART_RXEVENT_HT)
        gps_ht_events++;
    else if (evt == HAL_UART_RXEVENT_TC)
        gps_tc_events++;

    /* compute bytes received since last callback, accounting for DMA wrap */
    if (Size >= old_pos) {
        new_bytes = Size - old_pos;
    } else {
        new_bytes = DMA_LEN - old_pos + Size;
    }

    gps_rx_bytes = new_bytes;
    gps_rx_events++;

    /* Timing and Maximums */
    uint32_t current_time = HAL_GetTick();
    time_delta = current_time - last_callback_time;
    last_callback_time = current_time;

    if (gps_rx_bytes > max_gps_rx_bytes)
        max_gps_rx_bytes = gps_rx_bytes;
    if (time_delta > max_time_delta && gps_rx_events > 1)
        max_time_delta = time_delta;

    // Update position for next interrupt
    old_pos = Size;
    if (old_pos >= DMA_LEN)
        old_pos = 0;

    /* toggle LED2 on every callback so we can see DMA is alive */
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

/* -----------------------
 * HAL_UARTEx_RxEventCallback -- this is the HAL weak function we override.
 * It fires on every DMA event (idle, half-transfer, transfer-complete).
 *
 * gps_on_rx_event() is the only call that actually matters for the driver.
 * The debug callback above can be removed once testing is done.
 * ----------------------- */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size) {
    if (huart == &huart5) {
        debug_uart_callback(huart, Size);  // stuff for debugging, delete once not needed
        gps_on_rx_event(Size);             // only thing needed for callback to work
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
    console_print("--- GPS driver starting ---\r\n");

    /* Step 1: initialize internal driver state.
     * Sets up the ring buffer, parser, and UART handler structs.
     * Does NOT start DMA yet. */
    gps_init(&huart5, &gps_rb, gps_rb_storage, gps_dma_buf);

    /* Step 2: configure the GPS module over UBX.
     *
     * Can make permenant or temporary changes by changing UBX_LAYERS in constants.h
     *
     * first boot: change gps baud to whatever needed:
     * 		- modify values in constants.h
     * 		- set reinit_baud == true
     *
     * Cannot change fix rate at the same time as baud rate
     *
     * Changing nmea outputs and fix rate:
     * 		- modify fix rate in constants.h
     * 		- set reinit_baud == false, reinit_nmea_rate == true
     *
     * No changes:
     * 		- set reinit_baud == false, reinit_nmea_rate == false
     *
     * Current setup:
     *      - GPS already has 115200 saved from a previous run
     *      - NMEA/rate config has already been applied and saved.
     *      - Both flags are false so no UBX commands are sent on this boot
     * */
    GPS_CfgStatus cfg = gps_configure(&huart5, /*reinit_baud*/ false, /*reinit_nmea_rate*/
                                      false);
    if (cfg != GPS_CFG_OK) {
        console_print("GPS config failed\r\n");
        Error_Handler();
    }

    /* Step 3: start DMA receive.
     * This must happen AFTER gps_configure() since gps_configure() uses
     * blocking HAL_UART_Transmit/Receive which conflicts with active DMA. */
    if (!gps_start()) {
        console_print("gps uart start error \r\n");
        Error_Handler();
    }
    console_print("gps uart started! \r\n");

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    uint32_t last_print = 0;
    char buf[128];  // for the sake of debugging

    while (1) {
        /* gps_process() drains the ring buffer and feeds bytes into the
         * NMEA parser. Call this as often as possible so sentences are
         * assembled and the fix struct stays current. */
        gps_process();

        /* print a status update once per second for debugging */
        if ((HAL_GetTick() - last_print) >= 1000) {
            last_print = HAL_GetTick();

            /* raw DMA event counters -- confirms the callback is firing */
            snprintf(buf, sizeof(buf),
                     "rx_events:%lu  bytes_last:%lu  idle:%lu  ht:%lu  tc:%lu\r\n",
                     gps_rx_events, gps_rx_bytes, gps_idle_events, gps_ht_events,
                     gps_tc_events);
            console_print(buf);

            /* latest GPS fix data */
            print_gps_fix();

            /* parser stats -- useful for confirming gps_configure() worked.
             * After a successful config you should see:
             *   seen == gga  (every sentence is GGA, none ignored)
             *   ign  == 0    (no stray RMC/GSV getting through)
             *   fail == 0    (no checksum errors) */
            print_gps_stats();

            // print new lines for some seperation and clarity in the console
            console_print("\r\n");
            console_print("\r\n");
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
    huart5.Init.BaudRate = 115200;
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
    HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin,
                      GPIO_PIN_RESET);

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
/* console_print -- blocking transmit to LPUART1 (ST-Link virtual COM port).
 * Used only for debugging. */
void console_print(const char* s) {
    if (!s)
        return;
    HAL_UART_Transmit(&hlpuart1, (uint8_t*)s, strlen(s), 2000);
}

/* print_gps_fix -- reads the latest fix from the driver and prints it.
 * "Elapsed time" tells you how many ms ago the last valid fix arrived,
 * which helps confirm the fix rate is working as expected. */
static void print_gps_fix(void) {
    GpsFix fix;
    char msg[128];

    if (!get_fix(&fix)) {
        console_print("GPS: no valid fix \r\n");
        return;
    }

    uint32_t age_ms = HAL_GetTick() - (uint32_t)fix.last_update_ms;

    snprintf(msg, sizeof(msg),
             "GPS fix: lat = %.6f | lon = %.6f | sats = %u | alt = %.1fm | age = %lums\r\n",
             fix.lat, fix.lon, fix.satellites_used, fix.alt, age_ms);
    console_print(msg);
}

/* print_gps_stats -- prints parser and drop counters.
 * Useful during initial testing to verify the GPS config is working and
 * no data is being lost. Safe to call in the main loop. */
static void print_gps_stats(void) {
    GpsStats stats;
    char msg[128];

    get_stats(&stats);
    size_t dropped = get_dropped_bytes();  // reads and clears the drop counter

    snprintf(msg, sizeof(msg),
             "stats: seen = %u  gga = %u  ign = %u  checksum_fail = %u  dropped = %u\r\n",
             (unsigned)stats.sentences_seen, (unsigned)stats.parsed_gga_count,
             (unsigned)stats.ignored_sentences,
             (unsigned)stats.checksum_failure, (unsigned)dropped);
    console_print(msg);
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
    while (1) {  // blink an led if in this function
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);

        for (volatile uint32_t i = 0; i < 1000000; i++) {
            __NOP();  // No Operation (do nothing)
        }
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
