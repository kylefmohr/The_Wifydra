#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <SPIFFS.h>
#include <FS.h>
#include <SD.h> 
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <HTTPUpdateServer.h>
#include <Update.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>
#include "driver/uart.h"

//uint8_t broadcastAddress[] = {0x30, 0xAE, 0xA4, 0x07, 0x0D, 0x64};
// Dom MAC = 08:A6:F7:23:A1:20
const byte domMAC[] = {0x08, 0xA6, 0xF7, 0x23, 0xA1, 0x20};
AsyncWebServer httpServer(80);
// Keep as all zeroes so we do a broadcast
const unsigned char broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//34:B7:DA:F9:36:E4 BOARD10
// An array of the sub boards' MAC addresses - useful for when we want to send a message to only one of the boards i.e. for firmware updates
const uint8_t subAddresses[12][6] = {
  {0x34, 0xB7, 0xDA, 0x59, 0x75, 0xF0}, // BOARD1
  {0x34, 0xB7, 0xDA, 0x59, 0x66, 0x80}, // BOARD2
  {0x40, 0x4C, 0xCA, 0x5B, 0x0D, 0x3C}, // BOARD3
  {0x34, 0xB7, 0xDA, 0x52, 0xAC, 0x88}, // BOARD4
  {0x34, 0xB7, 0xDA, 0x59, 0x5A, 0xB4}, // BOARD5
  {0x64, 0xE8, 0x33, 0x50, 0xAC, 0xD8}, // BOARD6
  {0xff, 0xff, 0x33, 0x50, 0xA0, 0xff}, // BOARD7
  {0x34, 0xB7, 0xDA, 0x59, 0x60, 0xA0}, // BOARD8
  {0x64, 0xE8, 0x33, 0x8B, 0x6D, 0x90}, // BOARD9
  {0x34, 0xB7, 0xDA, 0xF9, 0x36, 0xE4}, // BOARD10
  {0x34, 0xB7, 0xDA, 0x59, 0x71, 0x6C}, // BOARD11
  {0x34, 0xB7, 0xDA, 0x59, 0x62, 0x8C}  // BOARD12
};

// Only used when a firmware update is present on the SD card
#define SSID "wifydra"
#define PASSWORD "password"

SPIClass sdSPI;

#define SD_CS 5
#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23


#define LOG_FILE_PREFIX "/gpslog"
#define MAX_LOG_FILES INT_MAX
#define LOG_FILE_SUFFIX "csv"
#define LOG_COLUMN_COUNT 11
#define LOG_RATE 500
String currentFileName = "/sub1.bin";
unsigned int updatingBoard = 1;
File myFile;
char logFileName[13];
int totalNetworks = 0;
unsigned long lastLog = 0;
unsigned long start = 0;

#define GPS_TX 27
#define GPS_RX 22
#define GPS_EN 21
static const uint32_t GPSBaud = 9600;   // GPS module default baud rate is 9600
SoftwareSerial GPSSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;
//bool gpsReportSent = false;
unsigned long lastGPSAnnouncement = 0;

const String wigleHeaderFileFormat = "WigleWifi-1.4,appRelease=2.26,model=custom,release=0.0.0,device=The_Wifydra,display=3fea5e7,board=esp32s3,brand=Espressif";
char* log_col_names[LOG_COLUMN_COUNT] = {
  "MAC", "SSID", "AuthMode", "FirstSeen", "Channel", "RSSI", "Latitude", "Longitude", "AltitudeMeters", "AccuracyMeters", "Type"
};

#define TFT_DC 2
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_CLK 14
#define TFT_CS 15
#define TFT_BL 21
#define TFT_RST -1

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

esp_now_peer_info_t peerInfo;

typedef struct wardriving_data_t {
  char bssid[64];
  char ssid[32];
  char encryptionType[16];
  int32_t channel;
  int32_t rssi;
  int boardID;
} wardriving_data_t;

wardriving_data_t incomingWifiData;

typedef struct {
  bool gpsStatus;
  bool firmwareUpdateAvailable;
} status_t;

status_t status = {false, false};
bool shouldCheckForFirmwareUpdate = true;


uint32_t current_offset = 0;


const int MAX_RECENT_SSIDS = 10;
String recentSSIDs[MAX_RECENT_SSIDS];


// For both csv logging and printing, because the GPS library we're using likes to print times like 3:2:1 and dates like 3/2/2024
String pad(unsigned int value)
{
  if(value < 10)
  {
    return ("0" + String(value));
  }
  else
  {
    return(String(value));
  }
}


void printBoth(String message) {
  Serial.print(message);
  tft.print(message);
}

void printBothln(String message)
{
  Serial.println(message);
  tft.println(message);
}

// This only gets called before we have a GPS fix
void interpretGPSData(TinyGPSPlus gps)
{
  printBoth("Satellites visible: ");
  printBothln(String(gps.satellites.value()));
  printBoth("Current Time: ");
  printBoth(pad(gps.time.hour()));
  printBoth(":");
  printBoth(pad(gps.time.minute()));
  printBoth(":");
  printBothln(pad(gps.time.second()));
}


// If `dom.bin` is present on the SD card, this updates its firmware
void updateOwnFirmware() {
    File updateFile = SD.open("/dom.bin", FILE_READ);
    if (!updateFile) {
        Serial.println("Error opening update file.");
        return;
    }

    // Get file size
    size_t fileSize = updateFile.size();
    printBothln("File size: " + String(fileSize));

    // Start update process
    if (!Update.begin(fileSize)) {
        printBothln("Update failed to start.");
        Update.printError(Serial);
        return;
    }

    // Write firmware data to flash
    size_t written = Update.writeStream(updateFile);
    if (written == fileSize) {
        Serial.println("Update successful!");
        Serial.println("Restarting...");
        SD.remove("/dom.bin");
        ESP.restart();
    } else {
        Serial.printf("Update failed! (%u/%u)\n", written, fileSize);
        Update.printError(Serial);
    }

    updateFile.close();
}

// check for presence of sub1-12.bin on the SD card, if found, send a message to the board with the update, then setup a WiFi AP and a web server to serve the firmware update
void checkForFirmwareUpdate()
{
  
  if(SD.exists(currentFileName) && status.firmwareUpdateAvailable == false)
  {
    printBothln("Firmware update " + currentFileName + " found");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(IPAddress(10,0,0,254), IPAddress(10,0,0,254), IPAddress(255,255,255,0));
    WiFi.softAP(SSID, PASSWORD);
    if (!MDNS.begin(SSID)) { //http://esp32.local
      Serial.println("Error setting up MDNS responder!");
      while (1) {
        delay(1000);
    }}
    printBothln("SoftAP and MDNS responder started");
    httpServer.serveStatic("/update", SD, "/");
    // serve a list of files in the root directory (mostly for debugging)
    httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      File root = SD.open("/");
      String response = "<html><head><title>ESP32-S3 Tiny</title></head><body><h1>Files</h1><ul>";
      while (File entry = root.openNextFile()) {
        response += "<li><a href='update/";
        response += entry.name();
        response += "'>";
        response += entry.name();
        response += "</a></li>";
        entry.close();
      }
      response += "</ul></body></html>";
      request->send(200, "text/html", response);
    });
    httpServer.onNotFound([](AsyncWebServerRequest* request) {
      request->send(404, "text/plain", "Not found");
    });
    httpServer.begin();
    
    status.gpsStatus = false;
    status.firmwareUpdateAvailable = true;
    esp_err_t broadcastStatus = esp_now_send(subAddresses[updatingBoard - 1], (uint8_t *) &status, sizeof(status));
    // if our message isn't trasmitted properly, this helps us see why
    switch(broadcastStatus)
    {
      case ESP_OK:
        printBothln("Broadcast sent");
        break;
      case ESP_ERR_ESPNOW_NOT_INIT:
        printBothln("ESP_ERR_ESPNOW_NOT_INIT");
        break;
      case ESP_ERR_ESPNOW_ARG:
        printBothln("ESP_ERR_ESPNOW_ARG");
        break;
      case ESP_ERR_ESPNOW_INTERNAL:
        printBothln("ESP_ERR_ESPNOW_INTERNAL");
        break;
      case ESP_ERR_ESPNOW_NO_MEM:
        printBothln("ESP_ERR_ESPNOW_NO_MEM");
        break;
      case ESP_ERR_ESPNOW_NOT_FOUND:
        printBothln("ESP_ERR_ESPNOW_NOT_FOUND");
        break;
      case ESP_ERR_ESPNOW_IF:
        printBothln("ESP_ERR_ESPNOW_IF");
        break;
    }
  }
  else // if current file doesn't exist, get ready to check for the next one
  {
    updatingBoard += 1;
    if(updatingBoard > 12)
    {
      shouldCheckForFirmwareUpdate = false;
      WiFi.softAPdisconnect();
      MDNS.end();
      httpServer.end();
    }
    currentFileName = "/sub" + String(updatingBoard) + ".bin";
  }
  if(SD.exists("/dom.bin"))
  {
    updateOwnFirmware();
  }
  return;
}

// Sends a message to every board (via the broadcast address) using ESP-NOW
// telling them that we have a confirmed GPS fix, so they should start scanning for APs
void sendGPSAlert()
{
  status_t status;
  status.gpsStatus = true;
  status.firmwareUpdateAvailable = false;
  printBothln("GPS Fix acquired, sending ESP-NOW broadcast");
  esp_now_send(broadcastAddress, (uint8_t *) &status, sizeof(status));
}

// ensure we're not accidentially overwriting our .csv files
void updateFileName() {
  int i = 0;
  for (; i < MAX_LOG_FILES; i++) {
    memset(logFileName, 0, strlen(logFileName));
    sprintf(logFileName, "%s%d.%s", LOG_FILE_PREFIX, i, LOG_FILE_SUFFIX);
    if (!SD.exists(logFileName)) {
      Serial.println("we picked a new file name");
      Serial.println(logFileName);
      break;
    } else {
      Serial.print(logFileName);
      Serial.println(" exists");
    }
  }
  Serial.println("File name: ");
  Serial.println(logFileName);
}

// Prints the wigle header and the column names to the csv file
void printHeader() {
  File logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    int i = 0;
    logFile.println(wigleHeaderFileFormat);  // comment out to disable Wigle header
    for (; i < LOG_COLUMN_COUNT; i++) {
      logFile.print(log_col_names[i]);
      if (i < LOG_COLUMN_COUNT - 1)
        logFile.print(',');
      else
        logFile.println();
    }
    logFile.close();
  }
}

String printableDateTime()
{
  if(gps.date.isValid() && gps.time.isValid())
  {
    char dateBuffer[32];
    sprintf(dateBuffer, "%04d-%02d-%02d ", gps.date.year(), gps.date.month(), gps.date.day());
    char timeBuffer[32];
    sprintf(timeBuffer, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
    return String(dateBuffer) + String(timeBuffer);
  }
  else
  {
    return "No GPS fix";
  }
}


// Callback function that is executed any time data is received via ESP-NOW
// Determines based on message size whether this is wardriving info that should be written to the .csv,
// or if this is a board letting us know that it has completed updating its firmware, and we can now delete the
// firmware update file
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(wardriving_data_t)) {
    status_t *incomingStatus = (status_t *) incomingData;
    // This gets reached when a board has finished updating its firmware so we know that we can delete the firmware file
    if(!incomingStatus->gpsStatus && !incomingStatus->firmwareUpdateAvailable)
    {
      SD.remove(currentFileName);
      // updatingBoard += 1; // if we've received a message from a board that it's done updating, get ready to move on to the next one
      // if(updatingBoard > 12)
      // {
      //   shouldCheckForFirmwareUpdate = false;
      // }
      // currentFileName = "/sub" + String(updatingBoard) + ".bin";
      checkForFirmwareUpdate();
    }
    return;
    }
    File logFile = SD.open(logFileName, FILE_APPEND);
    memcpy(&incomingWifiData, incomingData, sizeof(incomingWifiData));

    String SSIDString = incomingWifiData.ssid;
    SSIDString.replace(",", ".");  // Commas in SSID break the CSV

    logFile.print(incomingWifiData.bssid); logFile.print(",");
    logFile.print(SSIDString); logFile.print(",");
    logFile.print(incomingWifiData.encryptionType); logFile.print(",");
    logFile.print(printableDateTime()); logFile.print(",");
    logFile.print(incomingWifiData.channel); logFile.print(",");
    logFile.print(incomingWifiData.rssi); logFile.print(",");
    logFile.print(gps.location.lat(), 8); logFile.print(",");
    logFile.print(gps.location.lng(), 8); logFile.print(",");
    logFile.print(gps.altitude.meters()); logFile.print(",");
    logFile.print(gps.hdop.value()); logFile.print(",");
    logFile.println("WIFI");
    logFile.close();

    // Shift the elements in the array to make room for the new SSID
    for (int i = MAX_RECENT_SSIDS - 1; i > 0; i--) {
        recentSSIDs[i] = recentSSIDs[i - 1];
    }

    // Store the new SSID at the first position in the array
    recentSSIDs[0] = incomingWifiData.ssid;

    totalNetworks += 1;
}

// Callback function that is executed any time data is sent via ESP-NOW
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void displayInfo() {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 10);
    tft.setTextWrap(true);
    tft.setTextSize(1);
    char gpsLat[20];
    char gpsLng[20];
    char gpsSat[20];
    dtostrf(gps.location.lat(), 5, 2, gpsLat);
    dtostrf(gps.location.lng(), 5, 2, gpsLng);
    dtostrf(gps.satellites.value(), 2, 0, gpsSat);
    String gpsInfo = "Lat: " + String(gpsLat) + " Long: " + String(gpsLng) + " Satellites: " + String(gpsSat);
    printBothln(gpsInfo);
    for (int i = 0; i < MAX_RECENT_SSIDS; i++) {
        printBothln(recentSSIDs[i]);
    }
    printBothln("Nets: " + String(totalNetworks));
}

void setup() {

  Serial.begin(115200);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sdSPI)) {
    printBothln("Card Mount Failed");
    while (1);
  }
  printBothln("SD Card mounted");
  

  
  WiFi.mode(WIFI_AP_STA);
  if (esp_now_init() != 0) {
    printBothln("Error initializing ESP-NOW");
    return;
  }
  Serial.println("My MAC address: " + WiFi.macAddress());

  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
  
  // iterate through the list of sub board MAC addresses and add them as peers
  for(int i = 0; i < 12; i++)
  {
    memcpy(peerInfo.peer_addr, subAddresses[i], 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      printBothln("Failed to add peer #" + String(i+1));
    }
    else
    {
      printBothln("Added peer #" + String(i+1));
    }
  }
    // also register the broadcast address as a peer so we can send messages to all boards at once
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to register broadcast address");
      return;
   }

  
  printBothln("ESP-NOW Initialized");
  
  pinMode(GPS_EN, OUTPUT);
  digitalWrite(GPS_EN, HIGH);
  GPSSerial.begin(GPSBaud);
  printBothln("GPS Serial initialized, waiting for GPS fix");
  checkForFirmwareUpdate();
}

void loop() {  
    char gpsData;
    if (GPSSerial.available()) {
      gps.encode(GPSSerial.read());
    }

    if(millis() - start > 5000)
    {
      start = millis();
      displayInfo();
      interpretGPSData(gps);
      // While we have a confirmed GPS fix, send all sub boards a packet letting them know
      // At most, once every 60 seconds (in case any subs go offline. etc.)
      if(gps.location.isValid() && (millis() - lastGPSAnnouncement > 60000))
      {
        if(lastGPSAnnouncement == 0)
        {
          // We only want to write a new .csv file once we have a GPS fix, since it isn't guaranteed that we'll get one
          updateFileName();
          printHeader();
        }
        sendGPSAlert();
        lastGPSAnnouncement = millis();
      }
      if(shouldCheckForFirmwareUpdate)
      {
        checkForFirmwareUpdate();
      }
    }
    
}