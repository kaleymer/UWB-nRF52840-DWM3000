#include "AU_DW3000.h"
#include <Adafruit_TinyUSB.h>   // keep TinyUSB CDC linked

// ----------------- System config -----------------
// REMOVE SPI_CRCEN for bring-up (most drivers don't implement SPI-CRC)
DW3000_SYS_CFG sys_cfg = { {
  /* byte0 */ (uint8_t)(/* no DIS_FCE, no DIS_FCS_TX */ /* keep */ PHR_6M8 /* data rate */ /* and ideally leave CIA_IPATOV = 0 for 4z */),
  /* byte1 */ (uint8_t)(/* no CIA_STS, no RXWTOE */ 0),
  /* byte2 */ 0,   // no PDOA
  /* byte3 */ 0
}};


// 6.8 Mbps, PSR=64 is OK; payload length set at runtime
DW3000_TX_FCTRL txfctrl = { 0, 1, 0, TX_FCTRL_TXPSR_128, 0, 0 };

//#define RUN_TX_MODE
#define RUN_RX_MODE

static uint8_t tx_msg[] = { 0xC5, 0, 'H','E','L','L','O',' ','A','D', 'O', 'M' };
static uint8_t rx_buffer[127] = {0};

static inline void led_tx_on()  { digitalWrite(LED_TX, LOW);  }   // active-low
static inline void led_tx_off() { digitalWrite(LED_TX, HIGH); }

static inline void led_rx_on()  { digitalWrite(LED_RX, LOW);  }   // active-low
static inline void led_rx_off() { digitalWrite(LED_RX, HIGH); }

void setup1() {
  pinMode(LED_TX, OUTPUT);
  digitalWrite(LED_TX, HIGH); // off
  pinMode(LED_RX, OUTPUT);
  digitalWrite(LED_TX, HIGH); // off
  pinMode(LED_BUILTIN, OUTPUT); // red LED to indicate power
  digitalWrite(LED_BUILTIN, LOW);

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

void loop1() {
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
  led_tx_on();
  DW3000_startTX();

  // wait for TX complete (TXFRS bit)
  uint32_t start_ms = millis();
  bool tx_ok = false;
  while ((millis() - start_ms) < 10) {  // ~10ms timeout
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

  delay(100);
  led_tx_off();
  delay(100);
#endif

#ifdef RUN_RX_MODE
  // Start RX
  DW3000_setSysStatus(SYS_STATUS_TXFRS_MASK | SYS_STATUS_RXFCG_MASK | 0x0000F800UL);
  led_rx_on();
  DW3000_startRX();

  uint32_t status_reg = 0;
  uint16_t frame_len  = 0;
  bool got = false;

  const uint32_t t0 = millis();
  while (millis() - t0 < 1000) {
    status_reg = DW3000_getSysStatus();
    if (status_reg & SYS_STATUS_RXFCG_MASK) { got = true; break; }
    delay(1);
  }
  if (!got) {
    Serial.println("RX TIMEOUT (no RXFCG)");
    Serial.print("SYS_STATUS after timeout = 0x"); 
    uint8_t raw[6];
    uint32_t lo = DW3000_getSysStatus32(raw);
    Serial.print("SYS_STATUS LO32 = 0x"); Serial.println(lo, HEX);
    Serial.print("SYS_STATUS RAW  = ");
    for (int i=0;i<6;i++){ Serial.print("0x"); Serial.print(raw[i], HEX); Serial.print(" "); }
    Serial.println();
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

  led_rx_off();
  delay(50);
#endif
}
