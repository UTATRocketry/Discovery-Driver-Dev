#include "lis3mdl_driver.h"

/**
 * @brief Write a single byte to a LIS3MDL register.
 * @param reg: Register address.
 * @param value: Value to write.
 */
void LIS3MDL_WriteRegister(uint8_t reg, uint8_t value) {
    uint8_t txData[2] = {reg & 0x7F, value};  // MSB 0 for write
    LIS3MDL_CS_LOW();
    HAL_SPI_Transmit(&hspi1, txData, sizeof(txData), HAL_MAX_DELAY);
    LIS3MDL_CS_HIGH();
}

/**
 * @brief Read a single byte from a LIS3MDL register.
 * @param reg: Register address.
 * @return The value read.
 */
uint8_t LIS3MDL_ReadRegister(uint8_t reg) {
    uint8_t txData = reg | 0x80;  // MSB 1 for read
    uint8_t rxData;
    LIS3MDL_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &txData, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &rxData, 1, HAL_MAX_DELAY);
    LIS3MDL_CS_HIGH();
    return rxData;
}

/**
 * @brief Read multiple bytes starting from a LIS3MDL register.
 * @param reg: Starting register address.
 * @param buffer: Buffer to store the data.
 * @param length: Number of bytes to read.
 */
void LIS3MDL_ReadRegisters(uint8_t reg, uint8_t *buffer, uint8_t length) {
    uint8_t txData = reg | 0x80;  // MSB 1 for read
    LIS3MDL_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &txData, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, buffer, length, HAL_MAX_DELAY);
    LIS3MDL_CS_HIGH();
}

/**
 * @brief Initialize the LIS3MDL.
 * @return 0 if initialization is successful, 1 otherwise.
 */
uint8_t LIS3MDL_Init(void) {
    uint8_t whoAmI = LIS3MDL_ReadRegister(WHO_AM_I_ADDR);
    if (whoAmI != WHO_AM_I_VALUE) {
        return 1;  // Device not found
    }

    // Configure CTRL_REG1: Enable temperature sensor, set high-performance mode, ODR=10Hz
    LIS3MDL_WriteRegister(CTRL_REG1_ADDR, 0x70);

    // Configure CTRL_REG2: Set full scale to ±12 gauss
    LIS3MDL_WriteRegister(CTRL_REG2_ADDR, 0x20);

    // Configure CTRL_REG3: Set continuous-conversion mode
    LIS3MDL_WriteRegister(CTRL_REG3_ADDR, 0x00);

    return 0;  // Initialization successful
}

/**
 * @brief Read magnetic field data from LIS3MDL.
 * @param x: Pointer to store X-axis magnetic field data.
 * @param y: Pointer to store Y-axis magnetic field data.
 * @param z: Pointer to store Z-axis magnetic field data.
 */
void LIS3MDL_ReadMagneticField(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t buffer[6];
    LIS3MDL_ReadRegisters(OUT_X_L_ADDR, buffer, 6);

    *x = (int16_t)((buffer[1] << 8) | buffer[0]);
    *y = (int16_t)((buffer[3] << 8) | buffer[2]);
    *z = (int16_t)((buffer[5] << 8) | buffer[4]);
}
