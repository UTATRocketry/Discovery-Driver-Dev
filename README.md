# RAB Comms — Remote Arming Board Communication System

## Overview

This workspace contains three STM32 embedded firmware projects implementing a command-and-control system for remotely arming and disarming two independent Remote Arming Boards (RABs) from a central Flight Management Computer (FMC). For testing purposes, an L4 was used to emulate one of the RAB's. When implementing, you should treat that board as a second G0.

```
FMC (Controller)
 ├── USART3 ──────────► RAB A (STM32G0B1RE)
 └── USART2 ──────────► RAB B (STM32L476RG)
```

Communication uses a custom 5-byte binary protocol over UART, with XOR checksum validation and ACK/NACK responses.

---

## Project Structure

```
RAB Comms/
├── FMC V.0.3/          — Flight Management Computer (central controller)
│   └── Core/
│       ├── Inc/FMC.h   — Protocol constants, types, function declarations
│       └── Src/
│           ├── main.c  — System init + command sequence
│           └── FMC.c   — Protocol send logic, UART routing, debug logging
│
├── RAB V.0.3/          — Remote Arming Board A (STM32G0B1RE, USART2)
│   └── Core/
│       ├── Inc/RAB.h
│       └── Src/
│           ├── main.c
│           └── RAB.c
│
└── RAB B V.0.3/        — Remote Arming Board B (STM32L476RG, USART3)
    └── Core/
        ├── Inc/RAB.h
        └── Src/
            ├── main.c
            └── RAB.c
```

---

## Protocol Frame Format

Every transaction uses a 5-byte frame in both directions:

| Byte | Field   | Description                                              |
|------|---------|----------------------------------------------------------|
| [0]  | SYNC    | Always `0xAA` — frame start marker                      |
| [1]  | ADDR    | `0x01` = RAB A, `0x02` = RAB B                          |
| [2]  | CMD/STS | Command (`0xA5` ARM / `0x5A` DISARM) or Status (`0x06` ACK / `0x15` NACK) |
| [3]  | SEQ     | Sequence number — FMC increments, RAB echoes back        |
| [4]  | CRC     | XOR of bytes [0]–[3]                                     |

---

## How the FMC Main File Works

**File:** `FMC V.0.3/Core/Src/main.c`

After peripheral initialisation (clocks, GPIO, UART, USB), `main()` executes a fixed test sequence:

```c
FMC_Proto_SendCommand(RAB_B, CMD_ARM);     // ARM RAB B
HAL_Delay(2500);
FMC_Proto_SendCommand(RAB_A, CMD_ARM);     // ARM RAB A
HAL_Delay(2500);
FMC_Proto_SendCommand(RAB_A, CMD_DISARM);  // DISARM RAB A
HAL_Delay(2500);
FMC_Proto_SendCommand(RAB_B, CMD_DISARM);  // DISARM RAB B
```

It then enters an empty infinite loop. To add custom command sequences, insert additional `FMC_Proto_SendCommand()` calls before or instead of this block.

`FMC_Proto_SendCommand(uint8_t addr, uint8_t cmd)` handles the full transaction:
1. Builds a 5-byte frame with the current sequence number and CRC.
2. Selects the correct UART peripheral based on `addr` (see `rab_uart()` in `FMC.c`).
3. Transmits the frame and waits up to the configured timeout for a 5-byte response.
4. Validates SYNC byte, CRC, and sequence number echo.
5. Logs the result (ACK / NACK / error) to PuTTY via LPUART1.

---

## RAB Main File Operation

**Files:** `RAB V.0.3/Core/Src/main.c` and `RAB B V.0.3/Core/Src/main.c`

After peripheral init, `main()`:
1. Calls `RAB_Proto_Init()` — reads pin **PA6** to determine the device's address.
2. Enters an infinite loop calling `RAB_Proto_Poll()` on every iteration.

`RAB_Proto_Poll()` on each call:
1. Clears any UART hardware error flags (overrun, noise, framing).
2. Waits up to 100 ms for a SYNC byte (`0xAA`).
3. If SYNC received, reads the remaining 4 bytes.
4. Discards the frame silently if the address does not match this RAB.
5. Validates CRC — sends NACK on failure.
6. Executes the command: ARM sets PA5 HIGH (LED4 on), DISARM sets PA5 LOW.
7. Sends a 5-byte ACK response echoing the sequence number.

---

## Switching Between RAB1 and RAB2

The two RAB firmware projects are **separate STM32CubeIDE projects** — there is no single `#ifdef` switch. Flash the correct project to the correct board:

| Board | Project to Flash | MCU | UART Peripheral | Baud Rate |
|-------|-----------------|-----|-----------------|-----------|
| RAB A | `RAB V.0.3`     | STM32G0B1RE | USART2 (PA2/PA3) | 209,700 baud, 7-bit |
| RAB B | `RAB B V.0.3`   | STM32L476RG | USART3 (PD8/PD9) | 115,200 baud, 8-bit |

(Note: The 209,700 baud, 7-bit seems suspicious but that's what my code had, and it worked)

### Key Code Differences Between RAB Projects

#### 1. HAL include (`RAB.h`)

**RAB V.0.3:**
```c
#include "stm32g0xx_hal.h"
```

**RAB B V.0.3:**
```c
#include "stm32l4xx_hal.h"
```
(Note: I included the L4 library because I simulated a RAB with an L4. When implementing, you should ignore this)

#### 2. UART handle (`RAB.c`)

**RAB V.0.3 — uses `huart2` throughout:**
```c
extern UART_HandleTypeDef huart2;

HAL_UART_Receive(&huart2, ...);
HAL_UART_Transmit(&huart2, ...);
__HAL_UART_CLEAR_OREFLAG(&huart2);
```

**RAB B V.0.3 — uses `huart3` throughout:**
```c
extern UART_HandleTypeDef huart3;

HAL_UART_Receive(&huart3, ...);
HAL_UART_Transmit(&huart3, ...);
__HAL_UART_CLEAR_OREFLAG(&huart3);
```

#### 3. PA6 pull resistor (`main.c` GPIO config)

The PA6 pin is read at startup to determine the RAB's address (`RAB_A = 0x01` if HIGH, `RAB_B = 0x02` if LOW).

**RAB V.0.3:** PA6 configured with `GPIO_PULLDOWN`

**RAB B V.0.3:** PA6 configured with `GPIO_PULLUP`

> If you need to override the detected address in software, modify `RAB_Proto_Init()` in `RAB.c` directly.

---

## FMC UART Routing

The FMC automatically selects the correct UART based on the target address via `rab_uart()` in `FMC V.0.3/Core/Src/FMC.c`:

| Target | UART Peripheral | Pins |
|--------|----------------|------|
| RAB A (`0x01`) | USART3 | PD8 (TX) / PD9 (RX) |
| RAB B (`0x02`) | USART2 | PA2 (TX) / PA3 (RX) |
| Debug log | LPUART1 | PG7 (TX) / PG8 (RX) → PuTTY |

---

## Debug Logging

Connect a USB-to-serial adapter to the FMC's LPUART1 pins and open a PuTTY session to monitor all transactions. Every command sent and response received is logged, including CRC errors, timeouts, NACK responses, and sequence mismatches.

---

## Building and Flashing

1. Open STM32CubeIDE and import the desired project (`FMC V.0.3`, `RAB V.0.3`, or `RAB B V.0.3`).
2. Build the project (**Project → Build All**).
3. Connect the target board via ST-Link.
4. Flash via **Run → Debug** or **Run → Run**.
5. Repeat for each board in the system.

Flash the FMC last so that the RABs are already listening when the FMC starts its command sequence.
