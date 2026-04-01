# Discovery-Dev
Development repo for Discovery avionics system.

Notes: 
- in main, <user code 4> contains a working function that sends a UBX CFG-VALSET configuration packet to the GPS module. Currenly configured to change baud rate but can also be used to change:
  - navigation rate (currently at 1Hz --> default)
  - disable nmea output
  - enable ubx binary output, etc. 
- main() in main.c also has some details on how to call the function to change the baud rate. Tested this, seems to work without issue
