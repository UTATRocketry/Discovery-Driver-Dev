#include "gps_config.h"

#include <string.h>

#include "constants.h"
#include "gps_interface.h"
#include "stm32l4xx_hal_uart.h"

/* ----------------------------------------------
 * UBX packet info
 * -------------------------------------------- */
/* CFG-VALSET: UBX Framing Constants class=0x06, id=0x8A (pg. 89) */
#define UBX_SYNC1 0xB5u
#define UBX_SYNC2 0x62u
#define UBX_CLASS_CFG 0x06u
#define UBX_ID_VALSET 0x8Au

#define UBX_MAX_PKT 128u

/* -----------------------
 * Configuration Key IDs (pg. 203+, Section 5.9)
 *
 * Each key is a 32-bit ID. The top nibble encodes the value size:
 *   0x2....... -> U1 (1 byte)
 *   0x3....... -> U2 (2 bytes, little-endian)
 *   0x4....... -> U4 (4 bytes, little-endian)
 *
 *   Using gps's UART1
 * ----------------------- */
// NMEA sentence output rates on GPS UART1 — all U1 (pg. 203+)
#define KEY_NMEA_DTM 0x209100A7u     // datum reference 					— disable
#define KEY_NMEA_GBS 0x209100DEu     // satellite fault detection 			— disable
#define KEY_NMEA_GGA 0x209100BBu     // lat, lon, fix quality, sats used 	— KEEP
#define KEY_NMEA_GLL 0x209100CAu     // lat/lon only, redundant with GGA  	— disable
#define KEY_NMEA_GNS 0x209100B6u     // multi-constellation fix data      	— disable
#define KEY_NMEA_GRS 0x209100CFu     // range residuals 					— disable
#define KEY_NMEA_GSA 0x209100C0u     // DOP + active sats                 	— disable
#define KEY_NMEA_GST 0x209100D4u     // 									— disable
#define KEY_NMEA_GSV 0x209100C5u     // satellites in view — main bloat source, disable
#define KEY_NMEA_RLM 0x20910401u     // return link message 				— disable
#define KEY_NMEA_RMC 0x209100ACu     // speed, course, time               	— disable
#define KEY_NMEA_VLW 0x209100E8u     // 									— disable
#define KEY_NMEA_VTG 0x209100B1u     // course over ground                	— disable
#define KEY_NMEA_ZDA 0x209100D9u     // time and date only                	— disable
#define KEY_NMEA_PUBX00 0x209100EDu  // 									— disable
#define KEY_NMEA_PUBX03 0x209100F2u  // 									— disable
#define KEY_NMEA_PUBX04 0x209100F7u  // 									— disable

// Measurement period in ms — U2 (pg. 223)
#define KEY_RATE_MEAS 0x30210001u

// UART1 baud rate — U4 (pg. 231)
#define KEY_UART1_BAUD 0x40520001u

/* ----------------------------------------------
 * Internal Helpers
 * -------------------------------------------- */
// UBX 8-bit Fletcher checksum over UBX-CFG-VALSET payload as defied by (u-blox, pg. 48)
static void ubx_checksum(const uint8_t* buffer, uint16_t len, uint8_t* ck_a,
                         uint8_t* ck_b) {
    uint8_t a = 0, b = 0;
    for (uint16_t i = 0; i < len; i++) {
        a += buffer[i];
        b += a;
    }
    *ck_a = a;
    *ck_b = b;
}

// Append a 4-byte key + 1-byte U1 value into buf at *pos
static void append_kv_u1(uint8_t* buf, uint16_t* pos, uint32_t key, uint8_t val) {
    buf[(*pos)++] = (uint8_t)(key & 0xFFu);
    buf[(*pos)++] = (uint8_t)((key >> 8) & 0xFFu);
    buf[(*pos)++] = (uint8_t)((key >> 16) & 0xFFu);
    buf[(*pos)++] = (uint8_t)((key >> 24) & 0xFFu);
    buf[(*pos)++] = val;
}

// Append a 4-byte key + 2-byte U2 value (little-endian) into buf at *pos
static void append_kv_u2(uint8_t* buf, uint16_t* pos, uint32_t key,
                         uint16_t val) {
    buf[(*pos)++] = (uint8_t)(key & 0xFFu);
    buf[(*pos)++] = (uint8_t)((key >> 8) & 0xFFu);
    buf[(*pos)++] = (uint8_t)((key >> 16) & 0xFFu);
    buf[(*pos)++] = (uint8_t)((key >> 24) & 0xFFu);
    buf[(*pos)++] = (uint8_t)(val & 0xFFu);
    buf[(*pos)++] = (uint8_t)((val >> 8) & 0xFFu);
}

// Append a 4-byte key + 4-byte U4 value (little-endian) into buf at *pos
static void append_kv_u4(uint8_t* buf, uint16_t* pos, uint32_t key,
                         uint32_t val) {
    buf[(*pos)++] = (uint8_t)(key & 0xFFu);
    buf[(*pos)++] = (uint8_t)((key >> 8) & 0xFFu);
    buf[(*pos)++] = (uint8_t)((key >> 16) & 0xFFu);
    buf[(*pos)++] = (uint8_t)((key >> 24) & 0xFFu);
    buf[(*pos)++] = (uint8_t)(val & 0xFFu);
    buf[(*pos)++] = (uint8_t)((val >> 8) & 0xFFu);
    buf[(*pos)++] = (uint8_t)((val >> 16) & 0xFFu);
    buf[(*pos)++] = (uint8_t)((val >> 24) & 0xFFu);
}

// Build and transmit a single UBX packet.
// payload points to the CFG-VALSET payload bytes (starting at version byte).
static GPS_CfgStatus ubx_send(UART_HandleTypeDef* huart, uint8_t cls,
                              uint8_t id, const uint8_t* payload, uint16_t payload_len) {
    uint8_t buf[UBX_MAX_PKT];
    uint16_t total = 6u + payload_len + 2u;

    if (total > UBX_MAX_PKT)
        return GPS_CFG_ERR_UART;

    // header (pg. 30)
    buf[0] = UBX_SYNC1;
    buf[1] = UBX_SYNC2;
    buf[2] = cls;
    buf[3] = id;
    buf[4] = (uint8_t)(payload_len & 0xFFu);
    buf[5] = (uint8_t)((payload_len >> 8) & 0xFFu);

    memcpy(&buf[6], payload, payload_len);

    // checksum covers [cls, id, len_lo, len_hi, payload] (pg. 32)
    uint8_t ck_a, ck_b;
    ubx_checksum(&buf[2], 4u + payload_len, &ck_a, &ck_b);
    buf[6u + payload_len] = ck_a;
    buf[6u + payload_len + 1u] = ck_b;

    return (HAL_UART_Transmit(huart, buf, total, 200u) == HAL_OK) ? GPS_CFG_OK : GPS_CFG_ERR_UART;
}

/* -----------------------
 * Baud Rate Change (only called if reinit_baud = true)
 *
 * Must be sent at the CURRENT baud (GPS_DEFAULT_BAUD).
 * The GPS switches immediately after receiving the packet.
 * Then reinit the STM32 UART to match.
 *
 * We skip waiting for ACK here — the GPS sends the ACK at the OLD baud
 * and switches before we can reliably read it. A short delay is enough.
 * ----------------------- */
static GPS_CfgStatus gps_set_baud(UART_HandleTypeDef* huart) {
    uint8_t payload[4u + 8u];
    uint16_t pos = 0;

    payload[pos++] = 0x00u;
    payload[pos++] = UBX_LAYERS;
    payload[pos++] = 0x00u;
    payload[pos++] = 0x00u;

    append_kv_u4(payload, &pos, KEY_UART1_BAUD, GPS_TARGET_BAUD);

    GPS_CfgStatus st = ubx_send(huart, UBX_CLASS_CFG, UBX_ID_VALSET, payload,
                                pos);
    if (st != GPS_CFG_OK) {
        console_print("gps_set_baud error\n\r");
        return st;
    }

    HAL_Delay(200u);  // give GPS time to commit before we switch STM32 side

    if (HAL_UART_DeInit(huart) != HAL_OK)
        return GPS_CFG_ERR_UART;
    huart->Init.BaudRate = GPS_TARGET_BAUD;
    if (HAL_UART_Init(huart) != HAL_OK)
        return GPS_CFG_ERR_UART;

    HAL_Delay(100u);  // settle at new baud before next packet

    return GPS_CFG_OK;
}

/* -----------------------
 * NMEA Filter + Fix Rate
 *
 * One CFG-VALSET packet with all settings (pg. 89).
 * Max 64 key-value pairs per packet (pg.  89)
 * We have 19 key-value pairs
 *
 * Payload breakdown:
 *   VALSET header:        	4 bytes
 *   18x NMEA keys (U1):   	18 x 5 = 90 bytes
 *   1x rate key  (U2):   	1 x 6 =  6 bytes
 *   Total:               	100 bytes
 * ----------------------- */
static GPS_CfgStatus gps_set_nmea_and_rate(UART_HandleTypeDef* huart) {
    uint8_t payload[128u];  // size == 128 (cause 100 byte payload)
    uint16_t pos = 0;

    // VALSET payload header (pg. 89-90)
    payload[pos++] = 0x00u;
    payload[pos++] = UBX_LAYERS;
    payload[pos++] = 0x00u;
    payload[pos++] = 0x00u;

    // NMEA output rates (pg. 223) — 1 = once per fix, 0 = off
    append_kv_u1(payload, &pos, KEY_NMEA_DTM, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_GBS, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_GGA, 1u);  // KEEP -- LAT/LON/SATS
    append_kv_u1(payload, &pos, KEY_NMEA_GLL, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_GNS, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_GRS, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_GSA, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_GST, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_GSV, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_RLM, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_RMC, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_VLW, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_VTG, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_ZDA, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_PUBX00, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_PUBX03, 0u);
    append_kv_u1(payload, &pos, KEY_NMEA_PUBX04, 0u);

    // fix rate: GPS_FIX_RATE_MS ms (default is 1000 ms per pg. 223)
    append_kv_u2(payload, &pos, KEY_RATE_MEAS, (uint16_t)GPS_FIX_RATE_MS);

    GPS_CfgStatus st = ubx_send(huart, UBX_CLASS_CFG, UBX_ID_VALSET, payload,
                                pos);
    if (st != GPS_CFG_OK) {
        console_print("gps_set_nmea_and_rate SEND error\n\r");
        return st;
    }

    // wait for ACK-ACK (0x05 0x01) or ACK-NAK (0x05 0x00) (pg. 53-54)
    //	if (!ubx_wait_ack(huart, UBX_CLASS_CFG, UBX_ID_VALSET, 1000u)) {
    //		console_print("gps_set_nmea_and_rate ACK error\n\r");
    //		return GPS_CFG_ERR_NAK;
    //	}

    return GPS_CFG_OK;
}

/* ----------------------------------------------
 * Functions
 * -------------------------------------------- */
// send the command. NOTE: CAN ONLY CHANGE BAUD OR NMEA/RATE SEPERATLY. Can't do both.
GPS_CfgStatus gps_configure(UART_HandleTypeDef* huart, bool reinit_baud,
                            bool reinit_nmea_rate) {
    GPS_CfgStatus st;

    // baud change. skip if GPS already at GPS_TARGET_BAUD (
    if (reinit_baud) {
        st = gps_set_baud(huart);
        if (st != GPS_CFG_OK)
            return st;
        else {
            console_print("baud changed!\n\r");
            return st;
        }
    }

    if (!reinit_nmea_rate) {  // do nothing, exit without error
        console_print("no commands sent to gps\n\r");
        return GPS_CFG_OK;
    }

    // NMEA filter + fix rate (at GPS_TARGET_BAUD)
    st = gps_set_nmea_and_rate(huart);
    if (st == GPS_CFG_OK)
        console_print("nmea output and fix rate changed! \n\r");
    else
        console_print("command to gps failed :( \n\r");

    return st;
}

/* -----------------------
 * ubx_wait_ack()
 *
 * Blocking receive loop. Parses incoming bytes until a complete UBX-ACK
 * packet is received or timeout expires.
 *
 * ACK-ACK frame: B5 62 | 05 01 | 02 00 | cls id | CK_A CK_B  (pg. 53)
 * ACK-NAK frame: B5 62 | 05 00 | 02 00 | cls id | CK_A CK_B  (pg. 54)
 *
 * NOTE: fuckass function currently doesn't work, idc enough to fix it.
 * commands still seem to work
 * ----------------------- */
typedef enum {
    STATE_SYNC1,
    STATE_SYNC2,
    STATE_CLASS,
    STATE_ID,
    STATE_LEN1,
    STATE_LEN2,
    STATE_PAYLOAD,
    STATE_CKA,
    STATE_CKB
} ubx_state_t;

bool ubx_wait_ack(UART_HandleTypeDef* huart, uint8_t cls, uint8_t id,
                  uint32_t timeout_ms) {
    uint32_t start = HAL_GetTick();
    uint8_t buf;
    ubx_state_t state = STATE_SYNC1;

    uint8_t msg_class, msg_id;
    uint16_t payload_len;
    uint8_t payload[2];
    uint8_t payload_idx = 0;
    uint8_t ck_a_rx, ck_b_rx;

    // checksum covers [class, id, len_lo, len_hi, payload[0], payload[1]]
    uint8_t ck_buf[6];

    while ((HAL_GetTick() - start) < timeout_ms) {
        if (HAL_UART_Receive(huart, &buf, 1, 2) != HAL_OK)
            continue;

        switch (state) {
            case STATE_SYNC1:
                if (buf == 0xB5u)
                    state = STATE_SYNC2;
                break;
            case STATE_SYNC2:
                state = (buf == 0x62u) ? STATE_CLASS : STATE_SYNC1;
                break;
            case STATE_CLASS:
                msg_class = ck_buf[0] = buf;
                state = STATE_ID;
                break;
            case STATE_ID:
                msg_id = ck_buf[1] = buf;
                state = STATE_LEN1;
                break;
            case STATE_LEN1:
                payload_len = ck_buf[2] = buf;
                state = STATE_LEN2;
                break;
            case STATE_LEN2:
                payload_len |= ((uint16_t)buf << 8);
                ck_buf[3] = buf;
                payload_idx = 0;
                // ACK/NAK payloads are always exactly 2 bytes -- skip anything else
                state = (payload_len == 2u) ? STATE_PAYLOAD : STATE_SYNC1;
                break;
            case STATE_PAYLOAD:
                payload[payload_idx] = ck_buf[4 + payload_idx] = buf;
                payload_idx++;
                if (payload_idx >= 2u)
                    state = STATE_CKA;
                break;
            case STATE_CKA:
                ck_a_rx = buf;
                state = STATE_CKB;
                break;
            case STATE_CKB: {
                ck_b_rx = buf;

                uint8_t calc_a, calc_b;
                ubx_checksum(ck_buf, 6u, &calc_a, &calc_b);

                // valid checksum + ACK class (0x05) + matches command's cls/id
                if ((calc_a == ck_a_rx) && (calc_b == ck_b_rx) && (msg_class == 0x05u) && (payload[0] == cls) && (payload[1] == id)) {
                    return (msg_id == 0x01u);  // 0x01 = ACK-ACK, 0x00 = ACK-NAK
                }

                state = STATE_SYNC1;  // not packet, keep looking
                break;
            }
        }
    }

    return false;  // timeout
}
