/*
 * ADS131B04Q1.c
 *
 *  Created on: Jan 31, 2025
 *      Author: William Gomez
 */
#include "ADS131B04Q1.h"

// Define external and global variables
SPI_HandleTypeDef *ADS131_hspi;
GPIO_TypeDef *ADS131pinLetter;
uint16_t ADS131_PIN_Number;

float ch_gain[4] = {1, 1, 1, 1};
float ch_offset[4] = {0, 0, 0, 0};

int ADS131B04Q1_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *GPIOpinLetter, uint16_t GPIO_PIN_Number) {
    ADS131_hspi = hspi;
    ADS131pinLetter = GPIOpinLetter;
    ADS131_PIN_Number = GPIO_PIN_Number;
    int status = 0;

    // begin by ensuring CS line is high impedance
    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_SET);

    // check if ADC is connected
    status = ADS131B04Q1_CheckConnection();
    if (status == HAL_ERROR) {
        return status; // if ADC is not connected, return 1
    }

    // drive CS line low
    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_RESET);

    // prepare write command
    // Write command: 011, register = 000011, number of extra registers =
    // 0000000
    uint8_t writeCommand[3] = {0b01100001, 0b10000000, 0};
    status = HAL_SPI_Transmit(ADS131_hspi, writeCommand, 3, HAL_MAX_DELAY);
    if (status == HAL_ERROR) return status;


    // Configure CLOCK: 0000 = reserved, 1111 = all channels enabled, 0 =
    // internal clk 00 = reserved, 111 = 16384 oversampling ratio, 10 = high
    // resolution
    uint8_t writeCLOCK[3] = {0b00001111, 0b00011110, 0};
    status = HAL_SPI_Transmit(ADS131_hspi, writeCLOCK, 3, HAL_MAX_DELAY);
    if (status == HAL_ERROR) return status;


    // drive CS line high
    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_SET);

    return 0;
}

int ADS131B04Q1_CheckConnection() {
    int status = 0;
    uint8_t RxData[3] = {0, 0, 0};

    // Read nnn nnnn plus 1 registers beginning at address a aaaa a: 101a aaaa
    // annn nnnn reading register 00h, only reading first register so nnn nnnn
    // should be 000 0000 command should be 1010, 0000, 0000, 0000
    uint8_t readCommand[3] = {0b10100000, 0b00000000, 0};
    // drive CS line low
    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_RESET);
    // send read command, which will read ID register
    status = HAL_SPI_Transmit(ADS131_hspi, readCommand, 3, HAL_MAX_DELAY);
    if (status == HAL_ERROR) return status;

    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_SET);

    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_RESET);
    status = HAL_SPI_Receive(ADS131_hspi, RxData, 3, HAL_MAX_DELAY);
    if (status == HAL_ERROR) return status;

    // drive CS line high
    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_SET);
    // If received data matches expected value, return 0
    if (RxData[0] == 0b01000100) {
        return 0;
    }
    return 1;
}

/*
 * @brief
 * Configures the ADC to use the specified oversampling ratio.
 * @param configNum
 * @return int
 */
int ADS131B04Q1_OSRConfig(int configNum) {
    /*
            Modulator oversampling ratio selection
            000b = 128
            001b = 256
            010b = 512
            011b = 1024
            100b = 2048
            101b = 4096
            110b = 8192
            111b = 16384
     */
    int status = 0;
    uint8_t OSRConfigByte = 0;
    if (configNum < 0 || configNum > 7)
        return 1;
    else
        OSRConfigByte = (uint8_t)configNum;

    OSRConfigByte = (OSRConfigByte << 2) + 2;

    // drive CS line low
    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_RESET);

    // prepare write command
    // Write command: 011, register = 000011, number of extra registers =
    // 0000000
    uint8_t writeCommand[3] = {0b01100001, 0b10000000, 0};
    status = HAL_SPI_Transmit(ADS131_hspi, writeCommand, 3, HAL_MAX_DELAY);
    if (status == HAL_ERROR) return status;


    // Configure CLOCK: 0000 = reserved, 1111 = all channels enabled
    // writeOSR byte: 0 = internal clk, 00 = reserved, writeOSR[2:4] = OSR
    // config, 10 = high resolution
    uint8_t writeOSR[3] = {0b00001111, 0, 0};
    writeOSR[1] = OSRConfigByte;
    status = HAL_SPI_Transmit(ADS131_hspi, writeOSR, 3, HAL_MAX_DELAY);
    if (status == HAL_ERROR) return status;

    // drive CS line high
    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_SET);

    return 0;
}

// helper function to convert ADC data from binary to a signed value
int32_t ADS131B04Q1_convertReading(uint32_t value) {
    // Extract the lower 24 bits
    int32_t signedValue = (int32_t)(value & 0xFFFFFF);
    // Sign extend if the 24th bit (bit 23) is set
    if (signedValue & 0x800000) {
        signedValue |= 0xFF000000;
        // Extend sign to 32-bit
    }
    return signedValue;
}

int ADS131B04Q1_ReadChannels(int32_t *channelReading1, int32_t *channelReading2, int32_t *channelReading3, int32_t *channelReading4) {
    int status = 0;

    uint8_t RxData[15] = {0};
    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_RESET);

    // A NULL command (nothing sent over a clocked line) automatically results
    // in ADC data being sent on MISO line
    status = HAL_SPI_Receive(ADS131_hspi, RxData, 15, HAL_MAX_DELAY);
    if (status == HAL_ERROR) return status;


    HAL_GPIO_WritePin(ADS131pinLetter, ADS131_PIN_Number, GPIO_PIN_SET);

    // clear channelReading before loading new data
    *channelReading1 = 0;
    // stitch bytes together into one 32-bit number
    *channelReading1 |= (uint32_t)RxData[3] << 16;
    *channelReading1 |= (uint32_t)RxData[4] << 8;
    *channelReading1 |= (uint32_t)RxData[5];

    // clear channelReading before loading new data
    *channelReading2 = 0;
    // stitch bytes together into one 32-bit number
    *channelReading2 |= (uint32_t)RxData[6] << 16;
    *channelReading2 |= (uint32_t)RxData[7] << 8;
    *channelReading2 |= (uint32_t)RxData[8];

    // clear channelReading before loading new data
    *channelReading3 = 0;
    // stitch bytes together into one 32-bit number
    *channelReading3 |= (uint32_t)RxData[9] << 16;
    *channelReading3 |= (uint32_t)RxData[10] << 8;
    *channelReading3 |= (uint32_t)RxData[11];

    // clear channelReading before loading new data
    *channelReading4 = 0;
    // stitch bytes together into one 32-bit number
    *channelReading4 |= (uint32_t)RxData[12] << 16;
    *channelReading4 |= (uint32_t)RxData[13] << 8;
    *channelReading4 |= (uint32_t)RxData[14];

    *channelReading1 = ADS131B04Q1_convertReading(*channelReading1);
    *channelReading2 = ADS131B04Q1_convertReading(*channelReading2);
    *channelReading3 = ADS131B04Q1_convertReading(*channelReading3);
    *channelReading4 = ADS131B04Q1_convertReading(*channelReading4);
    return 0;
}

int ADS131B04Q1_Calibrate(float x1, float y1, float x2, float y2,
                          int channelNum) {
    // slope
    float gain = (y2 - y1) / (x2 - x1);
    // y-intercept
    float offset = y1 - (gain)*x1;

    // depending on channel number, store in respective variable
    if (channelNum >= 1 && channelNum <= 4) {
        ch_gain[channelNum - 1] = gain;
        ch_offset[channelNum - 1] = offset;
    }

    return 0;
}

// function to apply gain and conversion to 2s complement and return float
float ADS131B04Q1_RawToVoltage(int channelReading, int channelNum) {
    float channelVoltage = ((float)channelReading / (1 << 23)) * 1.2;
    if (channelNum >= 1 && channelNum <= 4) {
        channelVoltage += ch_offset[channelNum - 1];
        channelVoltage *= ch_gain[channelNum - 1];
    }
    return channelVoltage;

}
