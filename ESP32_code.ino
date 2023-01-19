/////////////////////////////////
// FILE INCLUSION
////////////////////////////////
#include <map>
#include <BluetoothSerial.h>
#include <WiFiMulti.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h> 

/////////////////////////////////
// DEFINES
////////////////////////////////

// Bluetooth default discovery time
#define BT_WAITING_TIME  10000


// Start zone: for InfluxDB
#define DEVICE "ESP32" // for InfluxDB
#define DEVICE_NODE_1 "your_node_id"
// WiFi AP SSID
#define WIFI_SSID "your-ssid"
// WiFi password
#define WIFI_PASSWORD "your-wifi_password"

#define INFLUXDB_URL "your-url"
#define INFLUXDB_TOKEN "your-token"
#define INFLUXDB_ORG "your-org"
#define INFLUXDB_BUCKET "your-bucket"

// Time zone info
#define TZ_INFO "UTC2"

// End zone: for InfluxDB

#define WIFI_ATTEMPTS_MAX 100
/////////////////////////////////
// TYPES
////////////////////////////////

typedef enum {
  E_REQUEST_MEAN_MOISTURE_OVER_LAST_24H,
  E_REQUEST_MIN_MOISTURE_OVER_LAST_24H,

} t_influxDbQuery;

/////////////////////////////////
// GLOBAL VARIABLES
////////////////////////////////
// Start zone: for InfluxDB
WiFiMulti wifiMulti;

// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Declare Data point
Point soilMoistureSensor("soil_moisture");

// End zone: for InfluxDB


// To manage bluetooth connection with the other node
BluetoothSerial SerialBT;
esp_spp_sec_t sec_mask=ESP_SPP_SEC_NONE; // or ESP_SPP_SEC_ENCRYPT|ESP_SPP_SEC_AUTHENTICATE to request pincode confirmation
esp_spp_role_t role=ESP_SPP_ROLE_SLAVE; // or ESP_SPP_ROLE_MASTER
BTAdvertisedDevice *slaveDevice;

char  sendData = 'S';
int c = 0;
BTAddress addr;
int channel = 0;
int recvData = 0;
int previousRecvData = 0;
int c1;
int cnt = 0;
bool ESP_STATUS = true;
/////////////////////////////////
// MY FUNCTIONS
////////////////////////////////

void initializeWifiInfluxDb(void) {

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  // SW trace to inform about wifi connection
  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    cnt++;
    Serial.print(".");
    delay(100);

    if (cnt > WIFI_ATTEMPTS_MAX) {
      break;
    }
  }
  Serial.println();
  Serial.println("==== Wifi connection is established ====");

  // Accurate time is necessary for certificate validation and writing in batches
  // We use the NTP servers in your area as provided by: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
    ESP_STATUS = false;
  }

  // Add tags to the data point
  soilMoistureSensor.addTag("device", DEVICE_NODE_1);

  WiFi.disconnect();
}

/* ------------------------------------------------------------------------------ */

void sendDataToInfluxDb(void) {

  WiFi.reconnect();

  delay(BT_WAITING_TIME);
    
  // Store measured value into point
  soilMoistureSensor.addField("soilMoisture", recvData);
  
  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(soilMoistureSensor));
  
  // Check WiFi connection and reconnect if needed
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }
  
  // Write point
  if (!client.writePoint(soilMoistureSensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  // Clear fields for reusing the point. Tags will remain the same as set above.
  soilMoistureSensor.clearFields();
  recvData = 0;

  WiFi.disconnect();
  delay(BT_WAITING_TIME/2);
}

/* ------------------------------------------------------------------------------ */

// TODO: Correct query
void getMeanLevelOfMoisutreFromDb() {
  WiFi.reconnect();

  delay(BT_WAITING_TIME);
  // Query will find the mean soil_moisture value for last 24h
  String aggregate_query = "from(bucket: \"SoilMoisture_Bucket\")\n\
    |> range(start: - 1d)\n\
    |> filter(fn: (r) => r._measurement == \"soil_moisture\")\n\
    |> filter(fn: (r) => r._field == \"soilMoisture\")\n\
    |> filter(fn: (r) => r.device == \"Node_Slave\")\n\
    |> aggregateWindow(every: 1d, fn: mean, createEmpty: false)";

  Serial.printf("Built query: %s\n", aggregate_query);
  FluxQueryResult aggregate_result = client.query(aggregate_query);
  // Iterate over rows.
  while (aggregate_result.next()) {
    
    // Get converted value for flux result column 'SSID'
    String ssid = aggregate_result.getValueByName("SSID").getString();
    
    Serial.print("Mean soil moisture over the last 24h: ");
    
    // Get value of column named '_value'
    long value = aggregate_result.getValueByName("_value").getLong();
    Serial.print(value);
    
    // Get value for the _time column
    FluxDateTime time = aggregate_result.getValueByName("_time").getDateTime();
    
    String timeStr = time.format("%F %T");
    
    Serial.print(" at ");
    Serial.print(timeStr);
    
    Serial.println();
  }

  WiFi.disconnect();
  delay(BT_WAITING_TIME/2);
}

/* ------------------------------------------------------------------------------ */

// TODO: Correct query
void getMaxLevelOfMoisutreFromDb() {
  WiFi.reconnect();

  delay(BT_WAITING_TIME);
  // Query will find the min soil_moisture value for last 24h
  String aggregate_query = "from(bucket: \"SoilMoisture_Bucket\")\n\
    |> range(start: - 1d)\n\
    |> filter(fn: (r) => r._measurement == \"soil_moisture\")\n\
    |> filter(fn: (r) => r._field == \"soilMoisture\"\)n\
    |> filter(fn: (r) => r.device == \"Node_Slave\")\n\
    |> aggregateWindow(every: 1d, fn: max, createEmpty: false)";

  Serial.printf("Built query: %s\n", aggregate_query);
  FluxQueryResult aggregate_result = client.query(aggregate_query);
  // Iterate over rows.
  while (aggregate_result.next()) {
    
    // Get converted value for flux result column 'SSID'
    String ssid = aggregate_result.getValueByName("SSID").getString();
    
    Serial.print("Max soil moisture over the last 24h: ");
    
    // Get value of column named '_value'
    long value = aggregate_result.getValueByName("_value").getLong();
    Serial.print(value);
    
    // Get value for the _time column
    FluxDateTime time = aggregate_result.getValueByName("_time").getDateTime();
    
    String timeStr = time.format("%F %T");
    
    Serial.print(" at ");
    Serial.print(timeStr);
    
    Serial.println();
  }

  WiFi.disconnect();
  delay(BT_WAITING_TIME/2);
}

/* ------------------------------------------------------------------------------ */

void initializeBluetooth(void) {

  if (! SerialBT.begin("ESP3", true) ) {
    Serial.println("==== BluetoothSerial failed! ====");
    abort();
  }
  
  Serial.println("Starting discoverAsync...");
  
  BTScanResults* btDeviceList = SerialBT.getScanResults();

  if (SerialBT.discoverAsync( [](BTAdvertisedDevice* pDevice) {
      Serial.printf("Found a new device: %s\n", pDevice->toString().c_str());}
      )) {

    // Add a delay to make sure that discover has some results
    delay(BT_WAITING_TIME);

    // Stop discovering
    Serial.print("Stopping discoverAsync... ");
    SerialBT.discoverAsyncStop();
    Serial.println("discoverAsync stopped");

    // Wait again...
    delay((BT_WAITING_TIME / 2 ));

    
    if (btDeviceList->getCount() > 0) {
      String deviceName;
      Serial.printf("Found devices: %d\n", btDeviceList->getCount());
      for (int i=0; i < btDeviceList->getCount(); i++) {

        // Get BT object
        BTAdvertisedDevice *device = btDeviceList->getDevice(i);
        
        Serial.printf("BT device with MAC: %s. Name: %s. RSSI: %d\n", device->getAddress().toString().c_str(), device->getName().c_str(), device->getRSSI());
        if (device->getName() == "Node_Slave") {
          slaveDevice = btDeviceList->getDevice(i);
        }
        std::map<int,std::string> channels = SerialBT.getChannels(device->getAddress());
        Serial.printf("Scanned for services, found %d\n", channels.size());
        for(auto const &entry : channels) {
          Serial.printf("     channel %d (%s)\n", entry.first, entry.second.c_str());
        }
        
        if(channels.size() > 0) {
          addr = device->getAddress();
          channel=channels.begin()->first;
        }
      }

      // Connect to BT devices
      if(addr) {
        Serial.printf("connecting to %s - %d\n", addr.toString().c_str(), channel);
        SerialBT.connect(addr, channel, sec_mask);
      }
      else {
        Serial.println("Unable to connect");
      }
    } else {
      Serial.println("Didn't find any devices");
    }
  } else {
    Serial.println("Error on discoverAsync f.e. not workin after a \"connect\"");
    ESP_STATUS = false;
  }
   // Wait again...
   delay((BT_WAITING_TIME / 2 ));
   SerialBT.disconnect();
   // Wait again...
   delay((BT_WAITING_TIME / 2 ));
   SerialBT.end();
}

/* ------------------------------------------------------------------------------ */

void readSoilMoistureFromNodeSlave(void) {

  recvData = 0;
  // Activate Bluetooth
    if (! SerialBT.begin("ESP3", true) ) {
    Serial.println("==== BluetoothSerial failed! ====");
    abort();
  }
  
  // Wait again...
  delay((BT_WAITING_TIME / 2 ));
  
  // Connect to BT device
  Serial.printf("connecting to %s - %d\n", addr.toString().c_str(), channel);
  SerialBT.connect(addr, channel, sec_mask, role);
  
  // Wait again...
  delay(BT_WAITING_TIME);
 
  // Request data from the other node
  if(! SerialBT.isClosed() && SerialBT.connected()) {
    
    if (SerialBT.write((const uint8_t*) &sendData, sizeof(sendData)) != sizeof(sendData)) {
      Serial.println("tx: error");
    } else {
//      Serial.printf("tx: %c\n",sendData);
    }
    delay(BT_WAITING_TIME);
    Serial.println("Checking if SerialBT is available...");
    if (SerialBT.available()) {
      Serial.print("rx: ");
      while(SerialBT.available()) {
        c = SerialBT.read();
        if(c >= 0) {
//          Serial.println( (char) c);
          c1 = ((char) c) - 48;
          recvData = (recvData * 10) + ( c1);
//          Serial.printf("recvData: %d, c1: %d,  int c: %d, char c: %c\n", recvData, c1, c, (char) c);
        }
      }
      Serial.println();
//      Serial.printf("Final recvData is %d\n", recvData);      
    }
  }
  else {
    Serial.println("not connected");
    recvData = previousRecvData;
  }
  Serial.printf("Received data over BT: %d\n", recvData);
  previousRecvData = recvData;
  delay(BT_WAITING_TIME/2);
  SerialBT.disconnect();
  delay(BT_WAITING_TIME);
  SerialBT.end();
}

/////////////////////////////////
// Arduino: SETUP and LOOP
////////////////////////////////
void setup() {
  
  Serial.begin(9600);

  // Manage bluetooth connection
  initializeBluetooth();
  
  // Manage Wi-Fi
  initializeWifiInfluxDb();

  if (ESP_STATUS == false) {
    ESP.restart();
  }
  else {
    Serial.printf("==== Initialization step: COMPLETE ====\n");
  }

 delay(BT_WAITING_TIME/2);
}

/* ------------------------------------------------------------------------------ */
void loop() {

  readSoilMoistureFromNodeSlave();

  sendDataToInfluxDb();

  // TODO: Manage query
//  getMaxLevelOfMoisutreFromDb();
//  getMeanLevelOfMoisutreFromDb
  Serial.println("Waiting for 30s");
  delay(30000);
}

/* ------------------------------------------------------------------------------ */
