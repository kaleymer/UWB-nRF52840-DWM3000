#include "adom_port_platform.h"
#include <Arduino.h>

// == Pin mapping (edit if you wired differently) ==
static const int PIN_UWB_RST = D5;   // DW3000 RSTn
static const int PIN_UWB_IRQ = D6;   // DW3000 IRQ (active-high)

static volatile bool g_irq_flag = false;

static void irq_trampoline() {
  g_irq_flag = true;                 // keep ISR tiny; just flag
}

void deca_port_init() {
  pinMode(PIN_UWB_RST, OUTPUT);
  digitalWrite(PIN_UWB_RST, HIGH);   // deassert reset

  pinMode(PIN_UWB_IRQ, INPUT_PULLUP);                 // IRQ as input; pull-up if needed
  attachInterrupt(digitalPinToInterrupt(PIN_UWB_IRQ), // edge-sensitive IRQ
                  irq_trampoline, RISING);
}

void deca_reset_pulse() {
  digitalWrite(PIN_UWB_RST, LOW);  delay(2);
  digitalWrite(PIN_UWB_RST, HIGH); delay(2);
}

bool deca_irq_read() {
  return digitalRead(PIN_UWB_IRQ) == HIGH;
}

bool deca_irq_take() {
  noInterrupts();
  bool f = g_irq_flag;
  g_irq_flag = false;
  interrupts();
  return f;
}
