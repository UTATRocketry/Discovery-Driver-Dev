/*
 * File Name: RAB.c
 * Author: Sebastian Southworth
 * Description: RAB's firmware for the RAB arming protocol
 * Date: 2026-3-9
 */

#include "RAB.h"

/* USART2 handle defined in main.c */
extern UART_HandleTypeDef huart2;  /* USART2 — link to FMC (L4) */

/* This RAB's address, set once at startup by reading PA6 */
static uint8_t my_addr = 0U;

/* ------------------------------------------------------------------------- */
/*  Helpers                                                                  */
/* ------------------------------------------------------------------------- */

uint8_t RAB_Proto_ComputeCRC(const uint8_t *buf, uint8_t len)
{
    uint8_t crc = 0U;
    for (uint8_t i = 0U; i < len; i++)
    {
        crc ^= buf[i];
    }
    return crc;
}

/* Build and transmit a 5-byte response frame */
static HAL_StatusTypeDef send_response(uint8_t status_byte, uint8_t seq)
{
    uint8_t tx_buf[PROTO_FRAME_LEN];

    tx_buf[PROTO_IDX_SYNC]   = PROTO_SYNC;
    tx_buf[PROTO_IDX_ADDR]   = my_addr;
    tx_buf[PROTO_IDX_STATUS] = status_byte;
    tx_buf[PROTO_IDX_SEQ]    = seq;
    tx_buf[PROTO_IDX_CRC]    = RAB_Proto_ComputeCRC(tx_buf, PROTO_FRAME_LEN - 1U);

    return HAL_UART_Transmit(&huart2,
                             tx_buf,
                             PROTO_FRAME_LEN,
                             PROTO_TX_TIMEOUT_MS);
}

/* ------------------------------------------------------------------------- */
/*  Public API                                                               */
/* ------------------------------------------------------------------------- */

void RAB_Proto_Init(void)
{
    /* PA6 determines this RAB's identity on the bus */
    if (HAL_GPIO_ReadPin(SENSOR_IN_PORT, SENSOR_IN_PIN) == GPIO_PIN_SET)
    {
        my_addr = PROTO_ADDR_RAB_A;  /* PA6 HIGH -> RAB_A (0x01) */
    }
    else
    {
        my_addr = PROTO_ADDR_RAB_B;  /* PA6 LOW  -> RAB_B (0x02) */
    }
}

HAL_StatusTypeDef RAB_Proto_Poll(void)
{
    HAL_StatusTypeDef hal_status;
    uint8_t           sync;
    uint8_t           rx_buf[PROTO_FRAME_LEN];

    /* 0. HARDWARE ERROR RECOVERY
     * Clear Overrun (ORE), Noise (NE), and Framing (FE) error flags.
     * If the FMC sent data while the RAB was busy, the hardware locked up.
     * Clearing these flags forces the hardware to start listening again.
     */
    __HAL_UART_CLEAR_OREFLAG(&huart2);
    __HAL_UART_CLEAR_NEFLAG(&huart2);
    __HAL_UART_CLEAR_FEFLAG(&huart2);

    /* Safely reset the HAL internal state machine if it got stuck */
    if (huart2.ErrorCode != HAL_UART_ERROR_NONE)
    {
        huart2.ErrorCode = HAL_UART_ERROR_NONE;
        huart2.RxState = HAL_UART_STATE_READY;
    }

    /* 1. Wait for SYNC byte (one byte at a time for alignment) */
    hal_status = HAL_UART_Receive(&huart2, &sync, 1U, PROTO_RX_TIMEOUT_MS);
    if (hal_status != HAL_OK)
    {
        return hal_status;  /* HAL_TIMEOUT = idle, no traffic */
    }
    if (sync != PROTO_SYNC)
    {
        return HAL_OK;  /* Not a frame start — discard and re-poll */
    }

    /* 2. SYNC matched — receive remaining 4 bytes */
    rx_buf[PROTO_IDX_SYNC] = PROTO_SYNC;
    hal_status = HAL_UART_Receive(&huart2,
                                  &rx_buf[PROTO_IDX_ADDR],
                                  PROTO_FRAME_LEN - 1U,
                                  PROTO_RX_TIMEOUT_MS);
    if (hal_status != HAL_OK)
    {
        return HAL_OK;  /* Incomplete frame — discard */
    }

    /* 3. Address check — silently discard if not addressed to us */
    if (rx_buf[PROTO_IDX_ADDR] != my_addr)
    {
        return HAL_OK;
    }

    /* 4. Validate CRC */
    uint8_t expected_crc = RAB_Proto_ComputeCRC(rx_buf, PROTO_FRAME_LEN - 1U);
    if (rx_buf[PROTO_IDX_CRC] != expected_crc)
    {
        send_response(PROTO_NACK, rx_buf[PROTO_IDX_SEQ]);
        return HAL_OK;
    }

    /* 5. Execute command — only PA5 (arming pin / LD4) is driven */
    uint8_t cmd = rx_buf[PROTO_IDX_CMD];
    switch (cmd)
    {
        case PROTO_CMD_ARM:
            HAL_GPIO_WritePin(ARM_PORT, ARM_PIN, GPIO_PIN_SET);    /* PA5 high, LD4 on  */
            break;

        case PROTO_CMD_DISARM:
            HAL_GPIO_WritePin(ARM_PORT, ARM_PIN, GPIO_PIN_RESET);  /* PA5 low,  LD4 off */
            break;

        default:
            /* Unknown command — NACK */
            send_response(PROTO_NACK, rx_buf[PROTO_IDX_SEQ]);
            return HAL_OK;
    }

    /* 6. Send ACK with the same sequence number */
    send_response(PROTO_ACK, rx_buf[PROTO_IDX_SEQ]);

    return HAL_OK;
}
