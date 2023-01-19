/////////////////////////////////
// FILE INCLUSION
////////////////////////////////

#include <SoftwareSerial.h>  

/////////////////////////////////
// DEFINES
////////////////////////////////

#define C_BLUETOOTH_TX    2      // TX-O pin of bluetooth mate, Arduino D2
#define C_BLUETOOTH_RX    3      // RX-I pin of bluetooth mate, Arduino D3
#define C_BAUDRATE        9600   // Communication baudrate
#define E_INVALID_CMD     0xff
#define C_SENSOR_PIN      A0

/////////////////////////////////
// TYPES
////////////////////////////////

typedef enum {
  E_READ_MOISTURE_SENSOR_CMD = 0x12
} t_bluetoothCommand;

/////////////////////////////////
// GLOBAL VARIABLES
////////////////////////////////

SoftwareSerial g_bluetooth(C_BLUETOOTH_TX, C_BLUETOOTH_RX);
char g_cmdId;

int g_previousSoilMoisture;
int g_currentSoilMoisture;

volatile int g_outputValue;
/////////////////////////////////
// FUNCTIONS
////////////////////////////////

int readMoistureSensor(void) {
  int l_sensorValue = analogRead(C_SENSOR_PIN);  // Read the analog value from sensor
  g_outputValue = map(l_sensorValue, 0, 1023, 255, 0); // map the 10-bit data to 8-bit data
//  analogWrite(C_LED_PIN, l_outputValue); // generate PWM signal
}


void setup() {

  // Confiure baudrate for both serial monitor and bluetooth-arduino
  Serial.begin(C_BAUDRATE);  // Begin the serial monitor at 9600bps
  g_bluetooth.begin(C_BAUDRATE);  // Start bluetooth serial at 9600

  // Set current and previous soil moisture to 0
  g_currentSoilMoisture = 0;
  g_previousSoilMoisture = 0;
}

void loop()
{
//  if(bluetooth.available())  // If the bluetooth sent any characters
//  {
//    // Send any characters the bluetooth prints to the serial monitor
//    Serial.print((char)bluetooth.read());  
//  }
//  if(Serial.available())  // If stuff was typed in the serial monitor
//  {
//    // Send any characters the Serial monitor prints to the bluetooth
//    bluetooth.print((char)Serial.read());
//  }
  // and loop forever and ever!

  // Check if connection is available
  if (g_bluetooth.available()) {

    // Read
    g_cmdId = (char) g_bluetooth.read();
    Serial.println(g_cmdId);
    switch (g_cmdId) {

      case 'S':
        // TODO: read data from moisture sensor
        readMoistureSensor();
        g_previousSoilMoisture = g_outputValue;
        Serial.println(g_outputValue);
        g_bluetooth.print(g_outputValue, DEC);
        break;

      default:
        // TODO: report g_previousSoilMoisture
        Serial.println(g_previousSoilMoisture);
        g_bluetooth.print(g_previousSoilMoisture, DEC);
        break;
    }
  }
}
