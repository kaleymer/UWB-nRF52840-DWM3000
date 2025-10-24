#pragma once
#include <stdbool.h>

// Init RST/IRQ pins and attach ISR
void deca_port_init();

// Hardware reset pulse to DW3000 (RSTn low then high)
void deca_reset_pulse();

// Read current IRQ pin level (true if asserted/high)
bool deca_irq_read();

// Atomically take (consume) the IRQ flag set by the ISR
bool deca_irq_take();
