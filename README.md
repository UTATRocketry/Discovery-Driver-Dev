# MS5611 SPI Driver

A lightweight C driver for the MS5611 barometric pressure and temperature sensor using SPI communication on STM32 microcontrollers.

## Hardware Requirements

- STM32 microcontroller (tested on STM32L4 series)
- MS5611 barometric pressure sensor
- SPI interface
- Optional: UART for debug output

## Pin Configuration

The following configuration was used during development (STM32CubeIDE 1.19.0):

| Peripheral | Pin  | Function       |
|------------|------|----------------|
| SPI1       | PA5  | SPI1_SCK       |
| SPI1       | PA6  | SPI1_MISO      |
| SPI1       | PA7  | SPI1_MOSI      |
| GPIO       | PC4  | Chip Select    |
| LPUART1    | PG7  | LPUART1_TX     |
| LPUART1    | PG8  | LPUART1_RX     |

**STM32CubeMX Settings:**
- SPI1: Full-Duplex Master, 8-bit data size
- LPUART1: Asynchronous, 8-bit data size
- PC4: GPIO Output (Chip Select)

## API Overview

### Data Structures

#### `MS5611_t`
Device handle containing SPI configuration and calibration coefficients.

```c
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *cs_port;
    uint16_t           cs_pin;
    uint16_t           prom[8];
} MS5611_t;
```

#### `BaroData`
Processed sensor readings.

```c
typedef struct {
    double temp;      // Temperature in °C
    double pressure;  // Pressure in mbar
    double altitude;  // Altitude in meters
} BaroData;
```

#### `MS5611_OSR_t`
Oversampling ratio options for balancing precision and conversion time.

```c
typedef enum {
    MS5611_OSR_256  = 0x00,  // Fastest, lowest precision
    MS5611_OSR_512  = 0x02,
    MS5611_OSR_1024 = 0x04,
    MS5611_OSR_2048 = 0x06,
    MS5611_OSR_4096 = 0x08   // Slowest, highest precision
} MS5611_OSR_t;
```

### Functions

#### `MS5611_Init`
Initializes the MS5611 sensor and reads calibration coefficients from PROM.

```c
HAL_StatusTypeDef MS5611_Init(MS5611_t *dev, 
                               SPI_HandleTypeDef *hspi, 
                               GPIO_TypeDef *port, 
                               uint16_t pin);
```

**Parameters:**
- `dev`: Pointer to device handle
- `hspi`: Pointer to SPI handle (e.g., `&hspi1`)
- `port`: GPIO port for chip select (e.g., `GPIOC`)
- `pin`: GPIO pin for chip select (e.g., `GPIO_PIN_4`)

**Returns:** `HAL_OK` on success

#### `MS5611_Readings`
Performs temperature and pressure conversions, applies compensation algorithms, and calculates altitude.

```c
BaroData MS5611_Readings(MS5611_t *dev, MS5611_OSR_t osr);
```

**Parameters:**
- `dev`: Pointer to initialized device handle
- `osr`: Oversampling ratio (affects precision and conversion time)

**Returns:** `BaroData` struct containing temperature, pressure, and altitude

#### `MS5611_Print`
Outputs formatted sensor data via UART for debugging.

```c
void MS5611_Print(UART_HandleTypeDef *huart, BaroData data);
```

**Parameters:**
- `huart`: Pointer to UART handle (e.g., `&hlpuart1`)
- `data`: Sensor data to print

## Usage Example

```c
#include "main.h"
#include "barometer.h"

// Declare device handles
MS5611_t baro1;
BaroData data;

// In main initialization section
MS5611_Init(&baro1, &hspi1, GPIOC, GPIO_PIN_4);

// In main loop
while(1) {
     
      data = MS5611_Readings(&baro1, MS5611_OSR_4096);
      MS5611_Print(&hlpuart1, data);
      HAL_Delay(500);
}
```

## Implementation Details

### Temperature Compensation
The driver implements both first-order and second-order temperature compensation as specified in the MS5611 datasheet for enhanced accuracy across the full temperature range (-40°C to +85°C).

### Altitude Calculation
Altitude is calculated using the barometric formula with sea-level pressure reference (1013.25 mbar):

```
altitude = 44330 × (1 - (P/P₀)^0.1902949)
```

### Conversion Timing
A 10ms delay is used after triggering ADC conversions, which is sufficient for all OSR settings up to 4096.

## File Structure

```
.
├── barometer.h          # Header file with API declarations
├── barometer.c          # Implementation file
└── README.md           # This file
```

## Contributors

Sebastian Southworth (Author)  
William Gomez (Editor)

## Acknowledgments

Based on the MS5611-01BA03 datasheet specifications.
