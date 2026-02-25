/**
 * @file accel.c
 * @brief Units in Gs
 */

#include "accel.h"

// Extern handles
extern SPI_HandleTypeDef hspi1;

// Internal offset storage
static vector_t offset = {0.0f, 0.0f, 0.0f};

// 49 mg/LSB = 0.049 g/LSB
#define SCALE_FACTOR  0.049f

static void adxl375_write_reg(uint8_t reg, uint8_t value) {
    uint8_t tx[2] = { reg, value };
    ADXL375_CS_LOW();
    HAL_SPI_Transmit(&hspi1, tx, 2, ADXL375_SPI_TIMEOUT);
    ADXL375_CS_HIGH();
}

static uint8_t adxl375_read_reg(uint8_t reg) {
    uint8_t tx = 0x80 | reg;
    uint8_t rx = 0;
    ADXL375_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &tx, 1, ADXL375_SPI_TIMEOUT);
    HAL_SPI_Receive(&hspi1, &rx, 1, ADXL375_SPI_TIMEOUT);
    ADXL375_CS_HIGH();
    return rx;
}

static void adxl375_read_multi(uint8_t start_reg, uint8_t* buffer, uint8_t len) {
    uint8_t tx = 0x80 | 0x40 | start_reg;
    ADXL375_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &tx, 1, ADXL375_SPI_TIMEOUT);
    HAL_SPI_Receive(&hspi1, buffer, len, ADXL375_SPI_TIMEOUT);
    ADXL375_CS_HIGH();
}


uint8_t Accel_Init(void) {
    char msg_buf[50];

    // Verify Device ID
    uint8_t devid = adxl375_read_reg(ADXL375_REG_DEVID);
    if (devid != ADXL375_DEVICE_ID) {
        // ADXL1375 Init failed
        return -1;
    }

    // Configure Settings
    adxl375_write_reg(ADXL375_REG_POWER_CTL, 0x00);   // Standby
    adxl375_write_reg(ADXL375_REG_DATA_FORMAT, 0x0B); // 4-wire, Full Res
    adxl375_write_reg(ADXL375_REG_BW_RATE, 0x0D);     // 800Hz
    adxl375_write_reg(ADXL375_REG_FIFO_CTL, 0x00);    // Bypass mode
    adxl375_write_reg(ADXL375_REG_POWER_CTL, 0x08);   // Measurement mode

    return 1;
}

void Accel_Calibrate(void) {

    float sum_x = 0, sum_y = 0, sum_z = 0;
    int samples = 50;
    uint8_t raw_bytes[6];
    int16_t raw16;

    for (int i = 0; i < samples; i++) {
        adxl375_read_multi(ADXL375_REG_DATAX0, raw_bytes, 6);

        // Read Raw X
        raw16 = (int16_t)((raw_bytes[1] << 8) | raw_bytes[0]);
        sum_x += (-raw16) * SCALE_FACTOR; // <--- Negated X

        // Read Raw Y
        raw16 = (int16_t)((raw_bytes[3] << 8) | raw_bytes[2]);
        sum_y += (-raw16) * SCALE_FACTOR; // <--- Negated Y

        // Read Raw Z
        raw16 = (int16_t)((raw_bytes[5] << 8) | raw_bytes[4]);
        sum_z += (-raw16) * SCALE_FACTOR; // <--- Negated Z

        HAL_Delay(10);
    }

    // Calculate Offsets
    // Target: X=0, Y=0, Z=-1.0 (gravity Vector Down)
    offset.x = 0.0f - (sum_x / samples);
    offset.y = 0.0f - (sum_y / samples);
    offset.z = -1.0f - (sum_z / samples); // Target is -1.0G
}

uint8_t Accel_Read(vector_t *data) {
    uint8_t raw_bytes[6];
    int16_t x_raw, y_raw, z_raw;

    adxl375_read_multi(ADXL375_REG_DATAX0, raw_bytes, 6);

    x_raw = (int16_t)((raw_bytes[1] << 8) | raw_bytes[0]);
    y_raw = (int16_t)((raw_bytes[3] << 8) | raw_bytes[2]);
    z_raw = (int16_t)((raw_bytes[5] << 8) | raw_bytes[4]);

    // Apply Inversion (-raw), Scale, and Offset
    data->x = ((-x_raw) * SCALE_FACTOR) + offset.x; // <--- Negated X
    data->y = ((-y_raw) * SCALE_FACTOR) + offset.y; // <--- Negated Y
    data->z = ((-z_raw) * SCALE_FACTOR) + offset.z; // <--- Negated Z

    return 0;
}
