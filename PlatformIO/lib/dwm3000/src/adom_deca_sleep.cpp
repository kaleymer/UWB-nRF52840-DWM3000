#include "adom_deca_sleep.h"
#include <Arduino.h>

void     deca_sleep_ms(uint32_t ms){ delay(ms); }
void     deca_sleep_us(uint32_t us){ delayMicroseconds(us); }
uint32_t deca_millis(){ return millis(); }
