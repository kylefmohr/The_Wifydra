#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <esp_wifi.h>
#include <ArduinoOTA.h>
#include <Update.h>


#ifdef BOARD_HAS_NEOPIXELS
#include <Adafruit_NeoPixel.h>
#define PIX_PIN 14
#define NUM_PIX 64
Adafruit_NeoPixel pix = Adafruit_NeoPixel(NUM_PIX, PIX_PIN, NEO_GRB + NEO_KHZ800);
#endif
#ifdef BOARD_HAS_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#ifndef PIX_PIN
#define PIX_PIN 14
#endif
#define NUM_PIX 1
Adafruit_NeoPixel pix = Adafruit_NeoPixel(NUM_PIX, PIX_PIN, NEO_GRB + NEO_KHZ800);
#endif
#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define WHITE 0xFFFFFF

// Keep as all zeroes so we do a broadcast

esp_now_peer_info_t peerInfo;


//say how many macs we should keep in the buffer to compare for uniqueness
#define mac_history_len 512

struct mac_addr {
  unsigned char bytes[6];
};
struct mac_addr mac_history[mac_history_len];
unsigned int mac_history_cursor = 0;

const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const byte domMAC[] = {0x08, 0xA6, 0xF7, 0x23, 0xA1, 0x20};
// Must match the receiver structure
typedef struct struct_message {

  char bssid[64];
  char ssid[32];
  char encryptionType[16];
  int32_t channel;
  int32_t rssi;
  int boardID;
} struct_message;


typedef struct {
  bool gpsStatus;
  bool firmwareUpdateAvailable;
} status_t;
bool shouldProceed = false;
bool firmwareUpdateAvailable = false;


status_t currentStatus;


String AP_SSID = "wifydra";
//**********
//**********
//This gets set by `create_all_images.py`
#ifdef BOARD1
#define BOARD_ID 1
#elif defined(BOARD2)
#define BOARD_ID 2
#elif defined(BOARD3)
#define BOARD_ID 3
#elif defined(BOARD4)
#define BOARD_ID 4
#elif defined(BOARD5)
#define BOARD_ID 5
#elif defined(BOARD6)
#define BOARD_ID 6
#elif defined(BOARD7)
#define BOARD_ID 7
#elif defined(BOARD8)
#define BOARD_ID 8
#elif defined(BOARD9)
#define BOARD_ID 9
#elif defined(BOARD10)
#define BOARD_ID 10
#elif defined(BOARD11)
#define BOARD_ID 11
#elif defined(BOARD12)
#define BOARD_ID 12
#else
#define BOARD_ID 0
#endif
//**********
//**********

String firmwareURL;
String PASSWORD = "password";



WebServer httpServer(80);


String AP;
String BSSIDchar;
String ENC;
String EncTy;
// Create a struct_message called myData
struct_message myData;
uint32_t current_offset = 0;

unsigned long lastTime = 0;
unsigned long timerDelay = 200;  // send readings timer
unsigned long start = 0;


// perform the actual update from a given stream
void performUpdate(Stream &updateSource, size_t updateSize) {
   if (Update.begin(updateSize)) {      
      size_t written = Update.writeStream(updateSource);
      if (written == updateSize) {
         Serial.println("Written : " + String(written) + " successfully");
      }
      else {
         Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
      }
      if (Update.end()) {
         Serial.println("OTA done!");
         if (Update.isFinished()) {
            Serial.println("Update successfully completed. Rebooting.");
         }
         else {
            Serial.println("Update not finished? Something went wrong!");
         }
      }
      else {
         Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }

   }
   else
   {
      Serial.println("Not enough space to begin OTA");
   }
}

void play_animation(uint32_t color = 0x0000FF)
{
  #ifdef BOARD_HAS_NEOPIXELS
  pix.setPixelColor(59, color);
  pix.setPixelColor(60, color);
  pix.show();
  delay(50);
     
  pix.show();
  delay(50);
    
  pix.setPixelColor(57, color);
  pix.setPixelColor(50, color);
  pix.setPixelColor(43, color);
  pix.setPixelColor(44, color);
  pix.setPixelColor(53, color);
  pix.setPixelColor(62, color);
  pix.show();
  delay(50);
         
  pix.show();
  delay(50);
      
  pix.setPixelColor(48, color);
  pix.setPixelColor(41, color);
  pix.setPixelColor(34, color);
  pix.setPixelColor(27, color);
  pix.setPixelColor(28, color);
  pix.setPixelColor(37, color);
  pix.setPixelColor(46, color);
  pix.setPixelColor(55, color);
  pix.show();
  delay(50);
          
  pix.show();
  delay(50);
      
  pix.setPixelColor(32, color);
  pix.setPixelColor(25, color);
  pix.setPixelColor(18, color);
  pix.setPixelColor(11, color);
  pix.setPixelColor(12, color);
  pix.setPixelColor(21, color);
  pix.setPixelColor(30, color);
  pix.setPixelColor(39, color);
  pix.show();
  delay(50);
        
  pix.show();
  delay(50);
      
  pix.setPixelColor(16, color);
  pix.setPixelColor(9, color);
  pix.setPixelColor(2, color);
  pix.setPixelColor(5, color);
  pix.setPixelColor(14, color);
  pix.setPixelColor(23, color);
  pix.show();
  delay(50);
  pix.clear();
  pix.show();
  #endif
  #ifdef BOARD_HAS_NEOPIXEL
  if(pix.getPixelColor(0) > 0)
  {
    pix.clear();
    pix.show();
    delay(100);
  }
  pix.fill(color);
  pix.show();
  #endif
  #ifdef LED_PIN
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  #endif
  
}

void firmwareUpdateMode(int id)
{
  Serial.println("Entering firmware update mode");
  if(firmwareUpdateAvailable)
  {
    play_animation(BLUE);
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    //block until we connect to SSID `wifydra` with password `password`
    if(WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(AP_SSID.c_str(), PASSWORD.c_str());
    }
    while(WiFi.status() != WL_CONNECTED)
    {
      delay(250);
      Serial.println("Connecting to WiFi network...");
    }
    Serial.println("Connected to WiFi network");
    //start the firmware update
    firmwareURL = "http://10.0.0.254/update/sub" + String(id) + ".bin";
    Serial.println("Downloading firmware from " + firmwareURL);

    HTTPClient httpClient;
    httpClient.begin(firmwareURL);

    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_OK) {
      // Found the firmware update, now download + install it
      WiFiClient firmwareClient = httpClient.getStream();
      size_t firmwareSize = httpClient.getSize();

      if (firmwareSize > 0) {
        Serial.println("Starting firmware update...");

        if (Update.begin(firmwareSize)) {
          if (Update.writeStream(firmwareClient) == firmwareSize) {
            if (Update.end(true)) {
              Serial.println("Firmware update successful");
              status_t status;
              status.firmwareUpdateAvailable = false;
              status.gpsStatus = false;
              esp_now_send(domMAC, (uint8_t*)&status, sizeof(status));
              ESP.restart();
            } else {
              Serial.println("Firmware update failed");
            }
          } else {
            Serial.println("Error writing firmware to ESP32");
          }
        } else {
          Serial.println("Error starting firmware update");
        }
      } else {
        Serial.println("Invalid firmware size");
      }
    } else {
      Serial.println("Failed to download firmware");
    }

    httpClient.end();
  }
}

boolean mac_cmp(struct mac_addr addr1, struct mac_addr addr2) {
  //Return true if 2 mac_addr structs are equal.
  for (int y = 0; y < 6; y++) {
    if (addr1.bytes[y] != addr2.bytes[y]) {
      return false;
    }
  }
  return true;
}

// Callback when data is sent
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

//void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
// Callback called when an unknown peer sends a message
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    Serial.println("ESPNow Packet Received");
    status_t* incomingStatus = (status_t*)incomingData;
    currentStatus.gpsStatus = incomingStatus->gpsStatus;
    currentStatus.firmwareUpdateAvailable = incomingStatus->firmwareUpdateAvailable;
    if(currentStatus.firmwareUpdateAvailable)
    {
      shouldProceed = false;
      firmwareUpdateAvailable = true;
    }
    else if(currentStatus.gpsStatus)
    {
      shouldProceed = true;
    }
  // }

}

void save_mac(unsigned char* mac) {
  //Save a MAC address into the recently seen array.
  if (mac_history_cursor >= mac_history_len) {
    mac_history_cursor = 0;
  }
  struct mac_addr tmp;
  for (int x = 0; x < 6; x++) {
    tmp.bytes[x] = mac[x];
  }

  mac_history[mac_history_cursor] = tmp;
  mac_history_cursor++;
  Serial.print("Mac len ");
  Serial.println(mac_history_cursor);
}

boolean seen_mac(unsigned char* mac) {
  //Return true if this MAC address is in the recently seen array.

  struct mac_addr tmp;
  for (int x = 0; x < 6; x++) {
    tmp.bytes[x] = mac[x];
  }

  for (int x = 0; x < mac_history_len; x++) {
    if (mac_cmp(tmp, mac_history[x])) {
      return true;
    }
  }
  return false;
}

void print_mac(struct mac_addr mac) {
  //Print a mac_addr struct nicely.
  for (int x = 0; x < 6; x++) {
    Serial.print(mac.bytes[x], HEX);
    Serial.print(":");
  }
}



String security_int_to_string(int security_type) {
  //Provide a security type int from WiFi.encryptionType(i) to convert it to a String which Wigle CSV expects.
  String authtype = "";
  switch (security_type) {
    case WIFI_AUTH_OPEN:
      authtype = "[OPEN]";
      break;

    case WIFI_AUTH_WEP:
      authtype = "[WEP]";
      break;

    case WIFI_AUTH_WPA_PSK:
      authtype = "[WPA_PSK]";
      break;

    case WIFI_AUTH_WPA2_PSK:
      authtype = "[WPA2_PSK]";
      break;

    case WIFI_AUTH_WPA_WPA2_PSK:
      authtype = "[WPA_WPA2_PSK]";
      break;

    case WIFI_AUTH_WPA2_ENTERPRISE:
      authtype = "[WPA2]";
      break;

    //Requires at least v2.0.0 of https://github.com/espressif/arduino-esp32/
    case WIFI_AUTH_WPA3_PSK:
      authtype = "[WPA3_PSK]";
      break;

    case WIFI_AUTH_WPA2_WPA3_PSK:
      authtype = "[WPA2_WPA3_PSK]";

    default:
      authtype = "";
  }

  return authtype;
}



void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  #if defined(BOARD_HAS_NEOPIXELS) || defined(BOARD_HAS_NEOPIXEL)
  pix.begin();
  pix.clear();
  pix.setBrightness(10);
  play_animation(WHITE);
  #endif
  #ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  #endif
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  // Register peer
  memcpy(peerInfo.peer_addr, domMAC, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("Added DOM MAC and broadcast address as peers");
  Serial.println("My MAC address: " + WiFi.macAddress());
  Serial.println("Waiting for GPS to come online");
  
}

void loop() {
  
  if(shouldProceed)
  {
    char Buf[50];
    char bufBSSID[64];
    char BufEnc[50];

    if ((millis() - lastTime) > timerDelay) {

      int n = WiFi.scanNetworks(false, true, false, 300, BOARD_ID);
      if (n == 0) {
        Serial.println("No networks found");
      } else {
        for (int8_t i = 0; i < n; i++) {
          if(WiFi.SSID(i).startsWith("wifydra"))
          {
            continue;
          }
          if (seen_mac(WiFi.BSSID(i))) {
            Serial.println("We've already seen it");
            Serial.println(myData.boardID);
            continue;
          }
          Serial.println("We havent seen it");
          String MacString = WiFi.BSSIDstr(i).c_str();
          MacString.toCharArray(bufBSSID, 64);
          strcpy(myData.bssid, bufBSSID);
          Serial.println(myData.bssid);
          String AP = WiFi.SSID(i);
          AP.toCharArray(Buf, 50);
          strcpy(myData.ssid, Buf);
          Serial.print("SSID: ");
          Serial.println(myData.ssid);
          switch (WiFi.encryptionType(i)) {
            case WIFI_AUTH_OPEN:
              EncTy = "Open";
              break;
            case WIFI_AUTH_WEP:
              EncTy = "WEP";
              break;
            case WIFI_AUTH_WPA_PSK:
              EncTy = "WPA PSK";
              break;
            case WIFI_AUTH_WPA2_PSK:
              EncTy = "WPA2 PSK";
              break;
            case WIFI_AUTH_WPA_WPA2_PSK:
              EncTy = "WPA/WPA2 PSK";
              break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
              EncTy = "WPA2 Enterprise";
              break;
            default:
              EncTy = "Unknown";
              break;
          }
          EncTy.toCharArray(BufEnc, 16);
          strcpy(myData.encryptionType, BufEnc);
          Serial.print("Encryption: ");
          Serial.println(myData.encryptionType);

          myData.channel = WiFi.channel(i);
          myData.rssi = WiFi.RSSI(i);
          myData.boardID = BOARD_ID;  //YOU NEED TO CHANGE THE BOARDID TO BE UNIQUE FOR EVERY SUB BVEFORE YOU FLASH IT. DONT DO IT HERE THOUGH
          Serial.println(myData.boardID);
          save_mac(WiFi.BSSID(i));
          esp_now_send(domMAC, (uint8_t*)&myData, sizeof(myData));
          play_animation(GREEN);
        }
        lastTime = millis();
      }
    }
  }
  else if(firmwareUpdateAvailable)
  {
    firmwareUpdateMode(BOARD_ID);
  }
  else if(millis() - start > 5000)
  {
    start = millis();
    Serial.println("Waiting for GPS to come online, my boardID is " + String(BOARD_ID) + " and my MAC address is " + WiFi.macAddress());
  }

}