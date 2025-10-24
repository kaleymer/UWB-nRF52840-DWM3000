#include "AU_DW3000.h"
#include <Adafruit_TinyUSB.h>   // keep TinyUSB CDC linked

// ----------------- System config -----------------
// REMOVE SPI_CRCEN for bring-up (most drivers don't implement SPI-CRC)
DW3000_SYS_CFG sys_cfg = {
  {
    // Byte 0
    (uint8_t)(DIS_DRXB | PHR_6M8 | CIA_IPATOV /* | SPI_CRCEN */),
    // Byte 1
    (uint8_t)(CIA_STS | RXWTOE /* | RXAUTR | AUTO_ACK if your driver needs them */),
    // Byte 2
    (uint8_t)(PDOA_MODE_3),   // or 0 if not using PDOA
    // Byte 3
    0
  }
};

// 6.8 Mbps, PSR=64 is OK; payload length set at runtime
DW3000_TX_FCTRL txfctrl = { 0, 1, 0, TX_FCTRL_TXPSR_64, 0, 0 };

//#define RUN_TX_MODE
#define RUN_TX_MODE

static uint8_t tx_msg[] = { 0xC5, 0, 'D','E','C','A','W','A','V','E' };
static uint8_t rx_buffer[127] = {0};

static inline void led_on()  { digitalWrite(LED_TX, LOW);  }   // active-low
static inline void led_off() { digitalWrite(LED_TX, HIGH); }

void setup() {
  pinMode(LED_TX, OUTPUT);
  digitalWrite(LED_TX, HIGH); // off

  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 2000) { delay(10); }

  Serial.println("\nDW3000 simple test (MODE0, no SPI-CRC)");

  // Optional: show config bytes
  Serial.print("SYS_CFG bytes: ");
  for (int i=0;i<4;i++){ Serial.print("0x"); Serial.print(sys_cfg.data[i], HEX); Serial.print(" "); }
  Serial.println();

  DW3000_getConfig();          // dump current config (if your driver prints it)
  DW3000_init(&sys_cfg);       // init & apply config
  DW3000_getDevID();           // expect 0xDECAxxxx printed by your driver
  DW3000_getEUI();
  DW3000_getOUI();
  DW3000_getConfig();

  delay(200);
}

void loop() {
#ifdef RUN_TX_MODE
#ifdef RUN_TX_MODE
  // --- enforce clean TX start sequence ---
  DW3000_clearIDLE();

  // clear any stale TX/RX events or error flags before starting
  DW3000_setSysStatus(SYS_STATUS_TXFRS_MASK | SYS_STATUS_RXFCG_MASK | 0x0000F800UL);

  // prepare payload and FCTRL
  txfctrl.tx_flen = sizeof(tx_msg);
  DW3000_writeTXData(sizeof(tx_msg), tx_msg, 0);
  DW3000_writeTXFCTRL(&txfctrl);

  // start transmit
  DW3000_startTX();

  // wait for TX complete (TXFRS bit)
  uint32_t start_ms = millis();
  bool tx_ok = false;
  while ((millis() - start_ms) < 50) {  // ~10ms timeout
    uint32_t status = DW3000_getSysStatus();
    if (status & SYS_STATUS_TXFRS_MASK) {
      tx_ok = true;
      break;
    }
    delayMicroseconds(200);
  }

  if (tx_ok) {
    DW3000_setSysStatus(SYS_STATUS_TXFRS_MASK); // clear TXFRS
    Serial.print("TX OK seq=");
    Serial.println(tx_msg[1]);
  } else {
    Serial.println("TX TIMEOUT (no TXFRS)");
  }

  // increment sequence field
  tx_msg[1]++;

  delay(300);
#endif


#endif

#ifdef RUN_RX_MODE
  // Start RX
  led_on();
  DW3000_startRX();

  uint32_t status_reg = 0;
  uint16_t frame_len  = 0;
  bool got = false;

  const uint32_t t0 = millis();
  while (millis() - t0 < 500) {          // 500 ms window to catch a frame
    status_reg = DW3000_getSysStatus();
    if (status_reg & SYS_STATUS_RXFCG_MASK) {
      got = true;
      break;
    }
    delay(1);
  }
  led_off();

  if (!got) {
    Serial.println("RX TIMEOUT (no RXFCG)");
    // optional: clear RX error/timeout bits if your driver requires it
    // DW3000_setSysStatus(SYS_STATUS_RXFCG_MASK | <RX ERR BITS>);
    delay(100);
    return;
  }

  // Clear RX good flag now that we've latched it
  DW3000_setSysStatus(SYS_STATUS_RXFCG_MASK);

  // Read frame length
  frame_len = DW3000_getRXFrameLength();
  Serial.print("RX len = "); Serial.println(frame_len);

  if (frame_len > 2 && frame_len <= 127) {
    // Often last 2 bytes are FCS (CRC) created by HW — subtract if your driver expects payload only
    uint16_t data_len = frame_len - 2;
    if (data_len > sizeof(rx_buffer)) data_len = sizeof(rx_buffer);
    memset(rx_buffer, 0, sizeof(rx_buffer));
    DW3000_getRXData(data_len, rx_buffer, 0);

    Serial.print("RX data: ");
    for (uint16_t i=0;i<data_len;i++){
      Serial.print("0x"); Serial.print(rx_buffer[i], HEX); Serial.print(" ");
    }
    Serial.println();

    // If the sender used ASCII payload, you can also print it as a string (safely):
    char s[128] = {0};
    uint16_t n = (data_len < 127) ? data_len : 127;
    memcpy(s, rx_buffer, n);
    Serial.print("RX text: "); Serial.println(s);
  } else {
    Serial.println("RX bad length");
  }

  delay(50);
#endif
}
