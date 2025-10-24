#include "adom_deca_spi.h"
#include <Arduino.h>
#include <SPI.h>

// == Pin mapping (edit if you wired differently) ==
static const int PIN_UWB_CS   = D7;   // DW3000 CSn
static const int PIN_UWB_SCK  = D10;  // SPI SCK
static const int PIN_UWB_MISO = D9;   // SPI MISO
static const int PIN_UWB_MOSI = D8;   // SPI MOSI

static SPIClass* uwbSPI = &SPI;

static inline void cs_low()  { digitalWrite(PIN_UWB_CS, LOW); }
static inline void cs_high() { digitalWrite(PIN_UWB_CS, HIGH); }

void deca_spi_init() {
  pinMode(PIN_UWB_CS, OUTPUT);
  digitalWrite(PIN_UWB_CS, HIGH);       // CS idle high
  uwbSPI->begin();
}

void deca_spi_write(const uint8_t* tx, size_t n) {
  cs_low();
  uwbSPI->beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0)); // 8 MHz, Mode 0
  uwbSPI->transfer((void*)tx, n);   // write burst
  uwbSPI->endTransaction();
  cs_high();
}

void deca_spi_rw(const uint8_t* tx, uint8_t* rx, size_t n) {
  cs_low();
  uwbSPI->beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0)); // 8 MHz, Mode 0
  uwbSPI->transfer((void*)tx, (void*)rx, n); // full-duplex
  uwbSPI->endTransaction();
  cs_high();
}
