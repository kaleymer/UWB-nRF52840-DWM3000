#pragma once
#include <stddef.h>
#include <stdint.h>

void deca_spi_init();                                // init SPI & CS pin
void deca_spi_write(const uint8_t* tx, size_t n);    // write-only burst
void deca_spi_rw(const uint8_t* tx, uint8_t* rx, size_t n); // full-duplex burst
