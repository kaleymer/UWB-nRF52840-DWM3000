#include <Arduino.h>
#include "DW3000.h"


//#define TX/RX_CODE
#define RX_CODE

static inline void led_tx_on()  { digitalWrite(LED_BLUE, LOW);  }   // active-low
static inline void led_tx_off() { digitalWrite(LED_BLUE, HIGH); }
static inline void led_tx_toggle()  { digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));  }  


static inline void led_rx_on()  { digitalWrite(LED_GREEN, LOW);  }   // active-low
static inline void led_rx_off() { digitalWrite(LED_GREEN, HIGH); }
static inline void led_rx_toggle()  { digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));  } 



#ifdef TX_CODE

#define TX_SENT_DELAY 500

static int tx_status; // Variable to store the current status of the receiver operation

void setup()
{
  Serial.begin(115200); // Init Serial
  DW3000.begin(); // Init SPI
  DW3000.hardReset(); // hard reset in case that the chip wasn't disconnected from power
  delay(200); // Wait for DW3000 chip to wake up

  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_BLUE, HIGH); // off
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, HIGH); // off
  pinMode(LED_BUILTIN, OUTPUT); // red LED to indicate power
  digitalWrite(LED_BUILTIN, LOW);

  if(!DW3000.checkSPI())
  {
    Serial.println("[ERROR] Could not establish SPI Connection to DW3000! Please make sure that all pins are set correctly.");
    while(100);
  }

  while (!DW3000.checkForIDLE()) // Make sure that chip is in IDLE before continuing 
  {
    Serial.println("[ERROR] IDLE1 FAILED\r");
    delay(1000);
  }

  DW3000.softReset(); // Reset in case that the chip wasn't disconnected from power
  delay(200); // Wait for DW3000 chip to wake up


  if (!DW3000.checkForIDLE())
  {
    Serial.println("[ERROR] IDLE2 FAILED\r");
    while (100);
  }

  
  DW3000.init(); // Initialize chip (write default values, calibration, etc.)
  DW3000.setupGPIO(); //Setup the DW3000s GPIO pins for use of LEDs
  Serial.println("[INFO] Setup is finished.");
  
  DW3000.configureAsTX(); // Configure basic settings for frame transmitting
}

void loop()
{
  DW3000.pullLEDHigh(2);
  DW3000.setTXFrame(507); // Set content of frame
  DW3000.setFrameLength(9); // Set Length of frame in bits

  DW3000.standardTX(); // Send fast command for transmitting
  led_tx_toggle();
  delay(10); // Wait for frame to be sent

  while (!(tx_status = DW3000.sentFrameSucc()))
  {
    Serial.println("[ERROR] Frame could not be sent succesfully!");
  };

  DW3000.clearSystemStatus(); // Clear event status

  Serial.println("[INFO] Sent frame successfully.");  
  DW3000.pullLEDLow(2);

  delay(TX_SENT_DELAY);
}

#endif 

#ifdef RX_CODE 

static int frame_buffer; // Variable to store the transmitted message
static int rx_status; // Variable to store the current status of the receiver operation

void setup()
{
  Serial.begin(115200); // Init Serial
  DW3000.begin(); // Init SPI
  DW3000.hardReset(); // hard reset in case that the chip wasn't disconnected from power
  delay(200); // Wait for DW3000 chip to wake up

  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_BLUE, HIGH); // off
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, HIGH); // off
  pinMode(LED_BUILTIN, OUTPUT); // red LED to indicate power
  digitalWrite(LED_BUILTIN, LOW);

  if(!DW3000.checkSPI())
  {
    Serial.println("[ERROR] Could not establish SPI Connection to DW3000! Please make sure that all pins are set correctly.");
    while(100);
  }

  while (!DW3000.checkForIDLE()) // Make sure that chip is in IDLE before continuing 
  {
    Serial.println("[ERROR] IDLE1 FAILED\r");
    delay(1000);
  }

  DW3000.softReset(); // Reset in case that the chip wasn't disconnected from power
  delay(200); // Wait for DW3000 chip to wake up


  if (!DW3000.checkForIDLE())
  {
    Serial.println("[ERROR] IDLE2 FAILED\r");
    while (100);
  }

  
  DW3000.init(); // Initialize chip (write default values, calibration, etc.)
  DW3000.setupGPIO(); //Setup the DW3000s GPIO pins for use of LEDs
  Serial.println("[INFO] Setup is finished.");
}

void loop() 
{
  DW3000.standardRX(); // Send command to DW3000 to start the reception of frames

  led_rx_toggle();

  while (!(rx_status = DW3000.receivedFrameSucc())) 
  {}; // Wait until frame was received
  
  if (rx_status == 1) { // If frame reception was successful
    DW3000.pullLEDHigh(2);
    frame_buffer = DW3000.read(0x12, 0x00); // Read RX_FRAME buffer0

    DW3000.clearSystemStatus();
    
    Serial.println("[INFO] Received frame successfully.");  
    Serial.print("Frame data (DEC): ");
    Serial.println(frame_buffer, DEC);  
	delay(50);
	DW3000.pullLEDLow(2);
  }
  else // if rx_status returns error (2)
  {
    Serial.println("[ERROR] Receiver Error occured! Aborting event.");
    DW3000.clearSystemStatus();
  }
}

#endif 