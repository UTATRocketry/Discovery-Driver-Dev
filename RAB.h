/*
 * File Name: RAB.h
 * Author: Sebastian Southworth
 * Description: RAB's header file for the RAB arming protocol
 * Date: 2026-3-9
 */

#ifndef INC_RAB_H_
#define INC_RAB_H_

#include "stm32g0xx_hal.h"
#include <stdint.h>

/* ========================================================================= */
/*  FMC <-> RAB Protocol V1  (must match FMC.h)                              */
/* ========================================================================= */

/* ---- Frame structure ---------------------------------------------------- */
#define PROTO_SYNC              ((uint8_t)0xAA)
#define PROTO_FRAME_LEN         5U

#define PROTO_IDX_SYNC          0U
#define PROTO_IDX_ADDR          1U
#define PROTO_IDX_CMD           2U
#define PROTO_IDX_STATUS        2U
#define PROTO_IDX_SEQ           3U
#define PROTO_IDX_CRC           4U

/* ---- RAB addresses ------------------------------------------------------ */
#define PROTO_ADDR_RAB_A        ((uint8_t)0x01)  /* PA6 HIGH */
#define PROTO_ADDR_RAB_B        ((uint8_t)0x02)  /* PA6 LOW  */

/* ---- Commands (FMC -> RAB) ---------------------------------------------- */
#define PROTO_CMD_ARM           ((uint8_t)0xA5)
#define PROTO_CMD_DISARM        ((uint8_t)0x5A)

/* ---- Response status codes (RAB -> FMC) --------------------------------- */
#define PROTO_ACK               ((uint8_t)0x06)
#define PROTO_NACK              ((uint8_t)0x15)

/* ---- Timeouts ----------------------------------------------------------- */
#define PROTO_TX_TIMEOUT_MS     100U
#define PROTO_RX_TIMEOUT_MS     100U

/* ---- Hardware pins ------------------------------------------------------ */

/* PA5 — arming output pin (also LD4 on NUCLEO-G0B1RE) */
#define ARM_PORT                GPIOA
#define ARM_PIN                 GPIO_PIN_5

/* PA6 — identity input pin (read once at startup to set address) */
#define SENSOR_IN_PORT          GPIOA
#define SENSOR_IN_PIN           GPIO_PIN_6

/* ---- Public API --------------------------------------------------------- */

/*
 * RAB_Proto_Init
 *
 * Read PA6 to determine this RAB's address.  Must be called once
 * before the main loop.
 */
void RAB_Proto_Init(void);

/*
 * RAB_Proto_Poll
 *
 * Block-wait for a command frame on USART2.
 *   - Addressed to us + valid CRC  -> execute command, send ACK
 *   - Addressed to us + bad CRC    -> send NACK
 *   - Addressed to another RAB     -> silently discard
 *
 * Returns HAL_OK on frame processed or discarded,
 *         HAL_TIMEOUT if no sync byte arrived.
 */
HAL_StatusTypeDef RAB_Proto_Poll(void);

/* Compute XOR checksum over `len` bytes of `buf`. */
uint8_t RAB_Proto_ComputeCRC(const uint8_t *buf, uint8_t len);

#endif /* INC_RAB_H_ */
