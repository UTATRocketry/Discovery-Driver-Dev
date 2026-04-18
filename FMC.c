/*
 * File Name: FMC.c
 * Author: Sebastian Southworth
 * Description: FMC firmware for RAB's arming protocol
 * Date: 2026-3-9
 */

#include "FMC.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* UART handles defined in main.c */
extern UART_HandleTypeDef hlpuart1;  /* LPUART1 — PuTTY debug console */
extern UART_HandleTypeDef huart2;    /* USART2  — link to RAB B       */
extern UART_HandleTypeDef huart3;    /* USART3  — link to RAB A       */

/* Sequence counter — incremented after every transaction */
static uint8_t seq_counter = 0U;

/* ------------------------------------------------------------------------- */
/*  Helpers                                                                  */
/* ------------------------------------------------------------------------- */

uint8_t FMC_Proto_ComputeCRC(const uint8_t *buf, uint8_t len)
{
    uint8_t crc = 0U;
    for (uint8_t i = 0U; i < len; i++)
    {
        crc ^= buf[i];
    }
    return crc;
}

/* Formatted log to PuTTY via LPUART1 */
static void log_putty(const char *fmt, ...)
{
    char    buf[80];
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0)
    {
        HAL_UART_Transmit(&hlpuart1,
                          (uint8_t *)buf,
                          (uint16_t)len,
                          PROTO_TX_TIMEOUT_MS);
    }
}

/* Return a short label for a RAB address */
static char rab_label(uint8_t addr)
{
    if (addr == PROTO_ADDR_RAB_A) return 'A';
    if (addr == PROTO_ADDR_RAB_B) return 'B';
    return '?';
}

/* Return the UART handle for the given RAB address */
static UART_HandleTypeDef *rab_uart(uint8_t addr)
{
    if (addr == PROTO_ADDR_RAB_B) return &huart2;  /* USART2 -> RAB B */
    return &huart3;                                 /* USART3 -> RAB A */
}

/* ------------------------------------------------------------------------- */
/*  Protocol transaction                                                     */
/* ------------------------------------------------------------------------- */

FMC_ProtoStatus FMC_Proto_SendCommand(uint8_t addr, uint8_t cmd)
{
    HAL_StatusTypeDef hal_status;
    uint8_t           tx_buf[PROTO_FRAME_LEN];
    uint8_t           rx_buf[PROTO_FRAME_LEN];
    uint8_t           seq_sent = seq_counter++;
    FMC_ProtoStatus   result;

    /* 1. Build command frame */
    tx_buf[PROTO_IDX_SYNC] = PROTO_SYNC;
    tx_buf[PROTO_IDX_ADDR] = addr;
    tx_buf[PROTO_IDX_CMD]  = cmd;
    tx_buf[PROTO_IDX_SEQ]  = seq_sent;
    tx_buf[PROTO_IDX_CRC]  = FMC_Proto_ComputeCRC(tx_buf, PROTO_FRAME_LEN - 1U);

    /* 2. Log outgoing frame */
    log_putty("[FMC] TX -> RAB_%c: cmd=%02X seq=%02X\r\n",
              rab_label(addr), cmd, seq_sent);

    /* 3. Transmit 5-byte command frame */
    UART_HandleTypeDef *huart = rab_uart(addr);
    hal_status = HAL_UART_Transmit(huart,
                                   tx_buf,
                                   PROTO_FRAME_LEN,
                                   PROTO_TX_TIMEOUT_MS);
    if (hal_status != HAL_OK)
    {
        log_putty("[FMC] TX FAIL\r\n");
        return FMC_PROTO_TX_FAIL;
    }

    /* 4. Wait for 5-byte response frame */
    hal_status = HAL_UART_Receive(huart,
                                  rx_buf,
                                  PROTO_FRAME_LEN,
                                  PROTO_RESP_TIMEOUT_MS);
    if (hal_status != HAL_OK)
    {
        log_putty("[FMC] RX TIMEOUT from RAB_%c\r\n", rab_label(addr));
        return FMC_PROTO_TIMEOUT;
    }

    /* 5. Validate SYNC */
    if (rx_buf[PROTO_IDX_SYNC] != PROTO_SYNC)
    {
        log_putty("[FMC] SYNC ERR: got %02X\r\n", rx_buf[PROTO_IDX_SYNC]);
        return FMC_PROTO_SYNC_ERR;
    }

    /* 6. Validate CRC */
    uint8_t expected_crc = FMC_Proto_ComputeCRC(rx_buf, PROTO_FRAME_LEN - 1U);
    if (rx_buf[PROTO_IDX_CRC] != expected_crc)
    {
        log_putty("[FMC] CRC ERR: exp=%02X got=%02X\r\n",
                  expected_crc, rx_buf[PROTO_IDX_CRC]);
        return FMC_PROTO_CRC_ERR;
    }

    /* 7. Validate SEQ */
    if (rx_buf[PROTO_IDX_SEQ] != seq_sent)
    {
        log_putty("[FMC] SEQ ERR: exp=%02X got=%02X\r\n",
                  seq_sent, rx_buf[PROTO_IDX_SEQ]);
        return FMC_PROTO_SEQ_ERR;
    }

    /* 8. Check ACK / NACK */
    if (rx_buf[PROTO_IDX_STATUS] == PROTO_ACK)
    {
        log_putty("[FMC] ACK from RAB_%c seq=%02X\r\n",
                  rab_label(rx_buf[PROTO_IDX_ADDR]), seq_sent);
        result = FMC_PROTO_OK;
    }
    else if (rx_buf[PROTO_IDX_STATUS] == PROTO_NACK)
    {
        log_putty("[FMC] NACK from RAB_%c seq=%02X\r\n",
                  rab_label(rx_buf[PROTO_IDX_ADDR]), seq_sent);
        result = FMC_PROTO_NACK;
    }
    else
    {
        log_putty("[FMC] UNKNOWN STATUS %02X\r\n", rx_buf[PROTO_IDX_STATUS]);
        result = FMC_PROTO_NACK;
    }

    return result;
}
