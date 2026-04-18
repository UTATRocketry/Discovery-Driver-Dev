/*
 * File Name: FMC.h
 * Author: Sebastian Southworth
 * Description: FMC's header file for RAB's arming protocol
 * Date: 2026-3-9
 */

#ifndef INC_FMC_H_
#define INC_FMC_H_

#include "stm32l4xx_hal.h"
#include <stdint.h>

/* ========================================================================= */
/*  FMC <-> RAB Protocol V1                                                  */
/* ========================================================================= */

/* ---- Frame structure ---------------------------------------------------- */
/*  [SYNC] [ADDR] [CMD/STATUS] [SEQ] [CRC]   (5 bytes in each direction)    */
#define PROTO_SYNC              ((uint8_t)0xAA)
#define PROTO_FRAME_LEN         5U

/* Byte offsets within a frame */
#define PROTO_IDX_SYNC          0U
#define PROTO_IDX_ADDR          1U
#define PROTO_IDX_CMD           2U      /* Command frame: command byte       */
#define PROTO_IDX_STATUS        2U      /* Response frame: ACK/NACK byte     */
#define PROTO_IDX_SEQ           3U
#define PROTO_IDX_CRC           4U

/* ---- RAB addresses (determined by PA6 identity pin on each G0) ---------- */
#define PROTO_ADDR_RAB_A        ((uint8_t)0x01)  /* PA6 HIGH */
#define PROTO_ADDR_RAB_B        ((uint8_t)0x02)  /* PA6 LOW  */

/* ---- Commands (FMC -> RAB) ---------------------------------------------- */
#define PROTO_CMD_ARM           ((uint8_t)0xA5)
#define PROTO_CMD_DISARM        ((uint8_t)0x5A)

/* ---- Response status codes (RAB -> FMC) --------------------------------- */
#define PROTO_ACK               ((uint8_t)0x06)  /* Command accepted */
#define PROTO_NACK              ((uint8_t)0x15)  /* Command rejected */

/* ---- Timeouts ----------------------------------------------------------- */
#define PROTO_TX_TIMEOUT_MS     100U
#define PROTO_RESP_TIMEOUT_MS   150U   /* Wait for full 5-byte response      */

/* ---- Transaction result ------------------------------------------------- */
typedef enum {
    FMC_PROTO_OK       = 0x00U,  /* ACK received, CRC valid, SEQ matched   */
    FMC_PROTO_NACK     = 0x01U,  /* NACK received (RAB rejected command)   */
    FMC_PROTO_TIMEOUT  = 0x02U,  /* No response within timeout             */
    FMC_PROTO_CRC_ERR  = 0x03U,  /* Response CRC mismatch                  */
    FMC_PROTO_SEQ_ERR  = 0x04U,  /* Response SEQ does not match command    */
    FMC_PROTO_TX_FAIL  = 0x05U,  /* HAL_UART_Transmit failed               */
    FMC_PROTO_SYNC_ERR = 0x06U   /* Response did not start with SYNC       */
} FMC_ProtoStatus;

/* ---- Public API --------------------------------------------------------- */

/*
 * FMC_Proto_SendCommand
 *
 * Build a 5-byte command frame addressed to `addr`, transmit on USART2
 * (RAB B) or USART3 (RAB A), wait for the 5-byte response, validate,
 * and log to PuTTY.
 *
 * Parameters:
 *   addr  - PROTO_ADDR_RAB_A or PROTO_ADDR_RAB_B
 *   cmd   - PROTO_CMD_ARM or PROTO_CMD_DISARM
 *
 * Returns: FMC_ProtoStatus indicating transaction outcome.
 */
FMC_ProtoStatus FMC_Proto_SendCommand(uint8_t addr, uint8_t cmd);

/* Compute XOR checksum over `len` bytes of `buf`. */
uint8_t FMC_Proto_ComputeCRC(const uint8_t *buf, uint8_t len);

#endif /* INC_FMC_H_ */
