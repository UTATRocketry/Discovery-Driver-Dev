#include "lis3mdl_driver.h"

int main(void) {
    HAL_Init();
    SystemClock_Config();

    // Initialize peripherals
    MX_GPIO_Init();
    MX_SPI1_Init();

    // Initialize LIS3MDL
    if (LIS3MDL_Init() != 0) {
        // Handle error: LIS3MDL not found
        while (1);
    }

    int16_t magX, magY, magZ;
    while (1) {
        LIS3MDL_ReadMagneticField(&magX, &magY, &magZ);
        // Process or display magnetic field data
        HAL_Delay(100);
    }
}
