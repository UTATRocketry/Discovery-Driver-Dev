# ADXL375 STM32 Driver

A lightweight SPI driver for the **ADXL375 high-g accelerometer** targeting the **STM32G0B1RETx** microcontroller, developed using STM32 HAL. Outputs calibrated acceleration data in units of **g (gravitational force)**.

---

## Data Structures

### `vector_t`

Defined in `accel.h`. Holds a 3-axis acceleration reading in units of **g**.

```c
typedef struct {
    float x;   // Acceleration along X-axis (g)
    float y;   // Acceleration along Y-axis (g)
    float z;   // Acceleration along Z-axis (g)
} vector_t;
```

**Example usage:**
```c
vector_t g;
Accel_Read(&g);
// g.x, g.y, g.z now hold calibrated values
// At rest flat: x ≈ 0.0, y ≈ 0.0, z ≈ -1.0
```

**Scale factor:** `49 mg/LSB` → `0.049 g/LSB`

**Gravity convention:** With the sensor flat and facing up, the Z-axis reads **-1.0 g** (gravity vector pointing down). X and Y read **0.0 g**.

---

## Reference

### `uint8_t Accel_Init(void)`

Initializes the ADXL375 over SPI. Verifies the device ID, configures data rate, output format, FIFO mode, and enables measurement mode.

| Parameter | Type | Description |
|-----------|------|-------------|
| *(none)*  | —    | —           |

**Returns:** `1` on success, `-1` if the device ID check fails.

**Configuration applied:**
- Standby → Full resolution data format → 800 Hz ODR → FIFO bypass → Measurement mode

**Example:**
```c
if (Accel_Init() != 0) {
    Error_Handler();
}
```

---

### `void Accel_Calibrate(void)`

Collects 50 samples with the sensor held flat and stationary, then computes per-axis offsets to zero out X/Y and target Z = -1.0 g (gravity). Offsets are stored internally as a `static vector_t` and applied automatically in every subsequent `Accel_Read()` call.

| Parameter | Type | Description |
|-----------|------|-------------|
| *(none)*  | —    | —           |

**Returns:** Nothing.

> **Keep the sensor completely flat and still during calibration.** A 500 ms settling delay is inserted before sampling begins.

**Example:**
```c
Accel_Calibrate();  // Must be called after Accel_Init()
```

---

### `uint8_t Accel_Read(vector_t *data)`

Reads a single 3-axis acceleration sample from the ADXL375, applies axis inversion, the 0.049 g/LSB scale factor, and the stored calibration offsets.

| Parameter | Type        | Description                                      |
|-----------|-------------|--------------------------------------------------|
| `data`    | `vector_t*` | Pointer to a `vector_t` struct to fill with results |

**Returns:** `0` (always succeeds if hardware is connected).

**Example:**
```c
vector_t g;
Accel_Read(&g);
```

---

## IOC / Hardware Configuration

Configured with **STM32CubeMX v6.15.0** targeting the **STM32G0B1RETx (LQFP64)**.

### SPI1 Settings

| Parameter         | Value                          |
|-------------------|--------------------------------|
| Mode              | Full-Duplex Master             |
| Data Size         | 8-bit                          |
| Clock Polarity    | HIGH (CPOL = 1)                |
| Clock Phase       | 2nd Edge (CPHA = 1) → **SPI Mode 3** |
| Baud Rate Prescaler | `/16` → **1.0 Mbits/s** at 16 MHz SYSCLK |
| First Bit         | MSB                            |
| NSS               | Software (manual CS via GPIO)  |
| TI Mode           | Disabled                       |
| CRC               | Disabled                       |

> The ADXL375 supports SPI Mode 3 (CPOL=1, CPHA=1), which matches this configuration.

### GPIO Pin Assignments

| Pin  | Function          | Notes                        |
|------|-------------------|------------------------------|
| PA0  | `GPIO_EXTI0`      | Interrupt input (rising edge), used for crash detection |
| PA1  | `SPI1_SCK`        | SPI clock                    |
| PA2  | `USART2_TX`       | Debug UART transmit          |
| PA3  | `USART2_RX`       | Debug UART receive           |
| PA4  | `GPIO_Output`     | **SPI CS (Chip Select)** — active LOW |
| PA5  | `GPIO_Output`     | LED / heartbeat toggle       |
| PA6  | `SPI1_MISO`       | SPI data in from sensor      |
| PA7  | `SPI1_MOSI`       | SPI data out to sensor       |

### USART2 Settings

| Parameter    | Value      |
|--------------|------------|
| Baud Rate    | 115200     |
| Word Length  | 8-bit      |
| Stop Bits    | 1          |
| Parity       | None       |
| Mode         | TX + RX    |

### Clock

- **SYSCLK:** 16 MHz (HSI, no PLL)
- **AHB / APB:** 16 MHz (no dividers)

---

## Register Map (ADXL375)

| Macro                        | Address | Description                  |
|------------------------------|---------|------------------------------|
| `ADXL375_REG_DEVID`          | `0x00`  | Device ID (reads `0xE5`)     |
| `ADXL375_REG_BW_RATE`        | `0x2C`  | Bandwidth and output data rate |
| `ADXL375_REG_POWER_CTL`      | `0x2D`  | Power control                |
| `ADXL375_REG_DATA_FORMAT`    | `0x31`  | Data format control          |
| `ADXL375_REG_DATAX0`         | `0x32`  | X-axis data LSB (burst start)|
| `ADXL375_REG_FIFO_CTL`       | `0x38`  | FIFO control                 |

---

## Example

```c
#include "accel.h"

// In main():
Accel_Init();       // Initialize sensor
Accel_Calibrate();  // Calibrate (keep flat & still)

vector_t g;
while (1) {
    Accel_Read(&g);
    // Use g.x, g.y, g.z
    HAL_Delay(100);
}
```

---

## Notes

- All three axes are **sign-inverted** in the driver (`-raw`) to match the board's physical mounting orientation. Remove the negation in `accel.c` if board orientation differs.
- `Accel_Calibrate()` must be called **after** `Accel_Init()` and with the sensor at rest.
- The SPI read command sets bit 7 (`0x80`) for read mode and bit 6 (`0x40`) for multi-byte burst reads, per the ADXL375 datasheet.