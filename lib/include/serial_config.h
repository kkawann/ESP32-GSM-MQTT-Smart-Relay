#pragma once

#ifdef ARDUINO_USB_CDC_ON_BOOT
#define DEBUG_SERIAL Serial
#else
#define DEBUG_SERIAL Serial0
#endif
