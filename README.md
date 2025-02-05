# Discovery-Driver-Dev
Driver development repo for Discovery avionics system.

# CRITICAL SETUP STEPS:
Ensure SPI connection to STM32 chip is configured in the following way:

Full-Duplex Master mode

Basic Parameters:

 Frame Format: Motorola
 
 Data Size: 8 bits
 
 First Bit: MSB First

Clock Params:

 Prescaler: 2
 
 Baud Rate: 500.0 Kbs 
 
 Clock Polarity: Low
 
 Clock Phase: 2 edge

Advanced Parameters:

 CRC Calculation: Disabled
 
 NSS Signal Type: Software
 

