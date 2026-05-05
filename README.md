# STM32 NEO-M9N GPS Driver

Non-blocking UART/DMA GPS driver for the u-blox NEO-M9N GNSS module, tested on STM32L4.

The driver configures the GPS module, receives NMEA data over UART using DMA, buffers incoming bytes in a software ring buffer, parses GGA sentences, and exposes the latest GPS fix to the application.

The current configuration is optimized for low memory use and reliable long-running operation.

---

## Quick Summary

 The driver is currently configured for:
 
```
115200 baud  |  5 Hz  |  GGA only  |  128-byte DMA  |  256-byte ring buffer
```

Expected steady-state:

```text
~76 bytes per fix
~5 GGA sentences per second
seen == gga
ign == 0
checksum_fail == 0
dropped == 0
```

If those conditions hold, the receive path, GPS configuration, buffering, and parser are all behaving correctly.

---

## Current Configuration

| Parameter            |          Value |
| -------------------- | -------------: |
| GPS module           | u-blox NEO-M9N |
| GPS UART baud        |         115200 |
| Fix/update rate      |           5 Hz |
| Measurement period   |         200 ms |
| Enabled NMEA output  |       GGA only |
| DMA RX buffer        |      128 bytes |
| Software ring buffer |      256 bytes |
| Total buffering      |      384 bytes |

Only GGA is enabled because the application currently needs:

* latitude
* longitude
* satellites used

GGA also contains altitude, fix quality, HDOP, UTC time, and checksum, but most of those fields are currently used only for validation/debugging.

With GGA-only output at 5 Hz, the GPS currently produces about **75–76 bytes every 200 ms**.

---

## Design Overview

The driver avoids blocking UART reads and avoids large static buffers.

Data flow:

```text
NEO-M9N GPS
    ↓ UART
STM32 UART RX DMA buffer
    ↓ HAL_UARTEx_RxEventCallback()
gps_on_rx_event()
    ↓
software ring buffer
    ↓ gps_process()
NMEA parser
    ↓
latest GpsFix
```

The DMA buffer receives bytes in the background. When new bytes arrive, the UART/DMA callback copies only the new region of the DMA buffer into the software ring buffer. The main loop then calls `gps_process()` to drain the ring buffer and parse complete NMEA sentences.

This design gives three important benefits:

1. GPS receive is non-blocking.
2. The parser does not need to run inside the interrupt.
3. Short bursts of GPS data can be absorbed without large memory usage.

---

## File Structure

```text
Core/
├── Inc/
│   ├── constants.h         buffer sizes, NMEA limits -- edit here first
│   ├── gps_config.h        GPS baud, fix rate, UBX layer bitmask
│   ├── gps_interface.h     public API (gps_init, gps_start, gps_process, get_fix)
│   ├── gps_parser.h        GpsFix and GpsStats structs, parser
│   ├── gps_uart.h          DMA + ring buffer handler struct
│   └── ring_buffer.h       generic FIFO
│
└── Src/
    ├── main.c              startup sequence, main loop, debug prints
    ├── gps_config.c        UBX CFG-VALSET packet builder and sender
    ├── gps_interface.c     ties together UART, DMA, ring buffer, and parser
    ├── gps_parser.c        NMEA sentence assembler and GGA parser
    ├── gps_uart.c          DMA callback handler, ring buffer write logic
    └── ring_buffer.c       generic FIFO implementation
```

---

## Important Files

### `constants.h` -- edit this first
 
Primary configuration file.
 
```c
#define GPS_TARGET_BAUD   115200U
#define GPS_DEFAULT_BAUD  38400U
#define GPS_FIX_RATE_MS   200U       // 200ms = 5Hz
#define DMA_LEN           128U
#define GPS_RB_LEN        256U
```
 
> Changing baud rate, fix rate, enabled NMEA messages, or buffer sizes should be done through constants/macros where possible. Avoid sending commands to the GPS as much as possible -- the checksum passes but ACK/NAK checking is not working. The whole config process requires a lot of debugging.

**GPS config save mode:**
```c
#define UBX_LAYER_RAM   0x01u  // UBX-defined value; do not change
#define UBX_LAYER_FLASH 0x04u  // UBX-defined value; do not change

/* Save mode:
 *   UBX_LAYER_RAM                         = temporary until reset/power loss
 *   UBX_LAYER_RAM | UBX_LAYER_FLASH       = apply now and save permanently
 */
#define UBX_LAYERS (UBX_LAYER_RAM | UBX_LAYER_FLASH)
```

### `gps_config.c / gps_config.h`
 
Handles GPS configuration over UBX messages: baud rate, fix rate, NMEA enable/disable, ACK/NAK handling, and optional save to non-volatile memory.
 
### `gps_interface.c / gps_interface.h`
 
Main driver interface. Initializes driver state, starts DMA, handles receive events, copies DMA bytes into the ring buffer, runs the parser, and exposes the latest fix and stats.
 
### `ring_buffer.c / ring_buffer.h`
 
Generic byte ring buffer used between DMA receive and the parser. If the buffer fills, new bytes are dropped and a drop counter is incremented.


### `main.c`

Responsibilities:
* initialize STM32 peripherals
* call GPS init/config/start functions
* call `gps_process()` continuously
* print debug information once per second
* implement the HAL UART RX event callback
Most CubeMX-generated code in this file is not part of the GPS driver logic.

---

## Startup Sequence

The intended startup sequence is:

```c
gps_init(&huart5, &gps_rb, gps_rb_storage, gps_dma_buf);

GPS_CfgStatus cfg = gps_configure(&huart5,
                                  reinit_baud,
                                  reinit_nmea_rate);

if (cfg != GPS_CFG_OK) {
    Error_Handler();
}

if (!gps_start()) {
    Error_Handler();
}
```
---

## GPS Configuration Modes

The driver supports three practical boot/config modes.

### 1. Normal Boot

Use when GPS settings are already saved.

```c
gps_configure(&huart5, false, false);
```

Expected saved GPS settings:

* 115200 baud
* 5 Hz update rate
* GGA only

No UBX reconfiguration is sent.

### 2. Reconfigure NMEA Output / Fix Rate

Use when baud is already correct but output settings need to change.

```c
gps_configure(&huart5, false, true);
```

This is used to apply settings such as:

* GGA enabled
* other NMEA sentences disabled
* fix rate set to 5 Hz

### 3. Reconfigure Baud

Use when the GPS is still at default baud or needs a baud change.

```c
gps_configure(&huart5, true, false);
```

> After changing GPS baud, `MX_UART5_Init` must be updated to match. If they do not match: received data looks like garbage, checksums fail, Error_Handler is hit, or DMA starts but never triggers.

---

## Runtime Requirements

```c
// call as often as possible in the main loop
gps_process();
```
Do not add blocking delays around `gps_process()`. If the application blocks too long, the ring buffer fills and incoming GPS bytes are dropped.

---

## DMA Callback
 
```c
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart == &huart5) {
        debug_uart_callback(huart, Size);  // debug only -- remove when done
        gps_on_rx_event(Size);             // required
    }
}
```
 
`gps_on_rx_event(Size)` is the only required call. `debug_uart_callback` only tracks counters and toggles LD2.

The current test build also calls: `debug_uart_callback(huart, Size);`
That function only tracks debug counters and toggles LD2. It is not required for normal driver operation.

---

## Debug Counters

### UART/DMA Counters

| Counter            | Meaning                          | Healthy Behavior                       |
| ------------------ | -------------------------------- | -------------------------------------- |
| `gps_rx_events`    | Total DMA receive callbacks      | continuously increases                 |
| `gps_rx_bytes`     | New bytes in latest callback     | often around 75–76 with GGA only       |
| `max_gps_rx_bytes` | Largest callback byte count seen | should stay well below `DMA_LEN`       |
| `time_delta`       | Time since previous callback     | around 200 ms for full GGA arrivals    |
| `max_time_delta`   | Largest callback gap             | should not grow during steady output   |
| `gps_idle_events`  | UART idle-line callbacks         | should increase regularly              |
| `gps_ht_events`    | DMA half-transfer callbacks      | may increase depending on DMA position |
| `gps_tc_events`    | DMA transfer-complete callbacks  | increases when DMA wraps               |

With the current configuration, IDLE events are the most meaningful because the GPS sends short periodic sentences rather than continuous data.

HT and TC callbacks are still useful because they provide extra chances to drain the DMA buffer.

### Parser Stats

| Stat            | Meaning                         | Healthy Behavior    |
| --------------- | ------------------------------- | ------------------- |
| `seen`          | Complete NMEA sentences seen    | increases about 5/s |
| `gga`           | Valid GGA sentences parsed      | should equal `seen` |
| `ign`           | Valid non-GGA sentences ignored | should be 0         |
| `checksum_fail` | Sentences with invalid checksum | should be 0         |
| `dropped`       | Ring-buffer bytes dropped       | should be 0         |

Healthy steady-state output:

```text
seen == gga
ign == 0
checksum_fail == 0
dropped == 0
```

---

## Dropped Bytes

Bytes are dropped when the ring buffer is full. The parser auto-recovers by resynchronizing on the next $ character, but the current fix is lost.

---

## Expected Test Results

With the current recommended settings, steady-state behavior should be:

```text
GPS baud:          115200
Fix rate:          5 Hz
NMEA output:       GGA only
Bytes per fix:     ~75–76
DMA buffer:        128 bytes
Ring buffer:       256 bytes
Dropped bytes:     0
Checksum failures: 0
Ignored sentences: 0
```

The GPS should be tested outdoors or near a clear sky view. The board may take several minutes to acquire a stable fix.

---

## Testing Procedure

1. Power the STM32 and GPS.
2. Place the GPS antenna outdoors or near a clear sky view. Wait for ~5 mins to start recieving data
3. Open the debug serial console.
4. Confirm startup messages print.
5. Confirm `gps_rx_events` increases.
6. Confirm `bytes_last` is usually around 75–76.
7. Confirm `seen` increases by about 5 per second.
8. Confirm `seen == gga`.
9. Confirm `ign == 0`.
10. Confirm `checksum_fail == 0`.
11. Confirm `dropped == 0`.
12. Wait for a valid fix.
13. Confirm latitude, longitude, and satellites-used update correctly.

---

## Debugging With STM32CubeIDE

Use Live Expressions to watch:

```c
gps_rx_events
gps_rx_bytes
max_gps_rx_bytes
time_delta
max_time_delta
gps_idle_events
gps_ht_events
gps_tc_events
```

---

## Current Buffer Sizing
 
```c
DMA_LEN    = 128   // must fit one full GGA sentence (~76 bytes) with margin
GPS_RB_LEN = 256   // holds ~3 GGA sentences of backlog
```
 
> These sizes are appropriate only for the current GGA-only 5 Hz config at 115200 baud.

---

## Known Issues
 
- **`ubx_wait_ack` not functional** -- ACK/NAK receive after UBX config does not work reliably. Replaced with a 500 ms delay. Not an issue for normal operation since GPS settings are saved to flash and `reinit_nmea_rate` is false on normal boots.
- **Fix age unreliable** -- the printed `age` field (`HAL_GetTick() - fix.last_update_ms`) produces incorrect values. At 5 Hz it should stay below ~200 ms. Treat this as a known debug-output issue. Lat, lon, satellite count, parser stats, and byte counts are all correct and should be checked independently.

---

## Production Integration Notes

* keep `gps_on_rx_event(Size)` in the UART callback
* keep `gps_process()` running frequently
* keep checksum validation
* keep dropped-byte tracking
* reduce or remove blocking debug prints
* remove LED debug toggling if not needed
* avoid blocking delays in the main loop/task
