# Discovery-Dev
Development repo for Discovery avionics system.

Notes: 
- main.c is completly broken (mixed with code from previous implementations). Will be updated once all the functions are finished.
- However, <user code 4> contains a working function that sends a UBX CFG-VALSET configuration packet to the GPS module. Currenly configured to change baud rate but can also be used to change:
  - navigation rate (currently at 1Hz --> default)
  - disable nmea output
  - enable ubx binary output, etc. 
- main() in main.c also has some details on how to call the function to change the baud rate. Tested this, seems to work without issue
- ring_buffer code is finished
- gps_uart code may or may not work. 
  - ring_buffer size of 2048 for efficency with nmea strings 
  - dma buffer much smaller (256 bytes?)
- gps_parser needs to be changed and debugged, causing most issues right now

To be implemented: 
- gps .h/.c (main interface to get data without dealing with any of this behind the scenes)
- gps_parser .h/.c to actually parse the data. Planning to use this struct.
```c
typedef struct {
    bool   valid;			// true if there's been a valid fix (checksum passes)
    double lat;				// lattitude
    double lon;				// longitude
    float  alt;				// altitude
    float  speedMps;		// meters per second. (converted from RMC knots)
    uint8_t satellitesUsed;	// from data sent by GGA
    uint32_t lastUpdateMs;	// feature implemented using HAL_GEtTick() --> can measure navigation rate (hz).
    						// Default navigation rate is set to be 1Hz on neom9n
} GpsFix;