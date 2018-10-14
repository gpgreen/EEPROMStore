#include "EEPROM.h"

#if defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
const size_t k_memory_length = 2048;
#else
const size_t k_memory_length = 1024;
#endif

MockEEPROM EEPROM(k_memory_length);
