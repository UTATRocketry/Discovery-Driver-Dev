// Put this under USER CODE BEGIN Includes
#include "../../Drivers/some_Driver/some_driver.h"
#include <stdio.h>
#include <stdarg.h>

// Put this under USER CODE BEGIN 0
void debug_printf(const char *fmt, ...) {
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  uint16_t i = 0;
  while(buffer[i] != '\0') {
    ITM_SendChar(buffer[i]);
    i++;
  }

}
// Put this under USER CODE BEGIN 2
 debug_printf("\n Starting X Driver Test\n");

  // Initialize device
  somedevice_init();
  
  // Start self test
  somedevice_self_test();

// Put this under USER CODE BEGIN 3
  somedevice_task();  // depending on the device this might not be needed
