#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#ifdef BOARD_HAS_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#define PIX_PIN 14
#define NUM_PIX 64
Adafruit_NeoPixel pix = Adafruit_NeoPixel(NUM_PIX, PIX_PIN, NEO_GRB + NEO_KHZ800);
#endif

// Keep as all zeroes so we do a broadcast
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

//say how many macs we should keep in the buffer to compare for uniqueness
#define mac_history_len 512

struct mac_addr {
  unsigned char bytes[6];
};
struct mac_addr mac_history[mac_history_len];
unsigned int mac_history_cursor = 0;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {

  char bssid[64];
  char ssid[32];
  char encryptionType[16];
  int32_t channel;
  int32_t rssi;
  int boardID;
} struct_message;

int boardID = -1;
//**********
//**********
//This gets set by `create_all_images.py`
#ifdef BOARD1
int boardID = 1;
#elif BOARD2
int boardID = 2;
#elif BOARD3
int boardID = 3;
#elif BOARD4
int boardID = 4;
#elif BOARD5
int boardID = 5;
#elif BOARD6
int boardID = 6;
#elif BOARD7
int boardID = 7;
#elif BOARD8
int boardID = 8;
#elif BOARD9
int boardID = 9;
#elif BOARD10
int boardID = 10;
#elif BOARD11
int boardID = 11;
#elif BOARD12
int boardID = 12;
#endif


String AP;
String BSSIDchar;
String ENC;
String EncTy;
// Create a struct_message called myData
struct_message myData;

unsigned long lastTime = 0;
unsigned long timerDelay = 200;  // send readings timer

void play_animation()
{
  #ifdef BOARD_HAS_NEOPIXEL
  pix.setPixelColor(59, 0x0000FF);
  pix.setPixelColor(60, 0x0000FF);
  pix.show();
  delay(50);
  // pix.setPixelColor(59, 0x000000);
  // pix.setPixelColor(60, 0x000000);
  // Next up, light pins 58, 51, 52, and 61
  pix.setPixelColor(58, 0x0000FF);
  pix.setPixelColor(51, 0x0000FF);
  pix.setPixelColor(52, 0x0000FF);
  pix.setPixelColor(61, 0x0000FF);
  pix.show();
  delay(50);
  // pix.setPixelColor(58, 0x000000);
  // pix.setPixelColor(51, 0x000000);
  // pix.setPixelColor(52, 0x000000);
  // pix.setPixelColor(61, 0x000000);
  // Next up, light 57, 50, 43, 44, 53, 62
  pix.setPixelColor(57, 0x0000FF);
  pix.setPixelColor(50, 0x0000FF);
  pix.setPixelColor(43, 0x0000FF);
  pix.setPixelColor(44, 0x0000FF);
  pix.setPixelColor(53, 0x0000FF);
  pix.setPixelColor(62, 0x0000FF);
  pix.show();
  delay(50);
  // pix.setPixelColor(57, 0x000000);
  // pix.setPixelColor(50, 0x000000);
  // pix.setPixelColor(43, 0x000000);
  // pix.setPixelColor(44, 0x000000);
  // pix.setPixelColor(53, 0x000000);
  // pix.setPixelColor(62, 0x000000);
  // Next up, light 56, 49, 42, 35, 36, 45, 54, 63
  pix.setPixelColor(56, 0x0000FF);
  pix.setPixelColor(49, 0x0000FF);
  pix.setPixelColor(42, 0x0000FF);
  pix.setPixelColor(35, 0x0000FF);
  pix.setPixelColor(36, 0x0000FF);
  pix.setPixelColor(45, 0x0000FF);
  pix.setPixelColor(54, 0x0000FF);
  pix.setPixelColor(63, 0x0000FF);
  pix.show();
  delay(50);
  // pix.setPixelColor(56, 0x000000);
  // pix.setPixelColor(49, 0x000000);
  // pix.setPixelColor(42, 0x000000);
  // pix.setPixelColor(35, 0x000000);
  // pix.setPixelColor(36, 0x000000);
  // pix.setPixelColor(45, 0x000000);
  // pix.setPixelColor(54, 0x000000);
  // pix.setPixelColor(63, 0x000000);
  // Next up, light 48, 41, 34, 27, 28, 37, 46, 55
  pix.setPixelColor(48, 0x0000FF);
  pix.setPixelColor(41, 0x0000FF);
  pix.setPixelColor(34, 0x0000FF);
  pix.setPixelColor(27, 0x0000FF);
  pix.setPixelColor(28, 0x0000FF);
  pix.setPixelColor(37, 0x0000FF);
  pix.setPixelColor(46, 0x0000FF);
  pix.setPixelColor(55, 0x0000FF);
  pix.show();
  delay(50);
  // pix.setPixelColor(48, 0x000000);
  // pix.setPixelColor(41, 0x000000);
  // pix.setPixelColor(34, 0x000000);
  // pix.setPixelColor(27, 0x000000);
  // pix.setPixelColor(28, 0x000000);
  // pix.setPixelColor(37, 0x000000);
  // pix.setPixelColor(46, 0x000000);
  // pix.setPixelColor(55, 0x000000);
  // Next up, light 40, 33, 26, 19, 20, 29, 38, 47
  pix.setPixelColor(40, 0x0000FF);
  pix.setPixelColor(33, 0x0000FF);
  pix.setPixelColor(26, 0x0000FF);
  pix.setPixelColor(19, 0x0000FF);
  pix.setPixelColor(20, 0x0000FF);
  pix.setPixelColor(29, 0x0000FF);
  pix.setPixelColor(38, 0x0000FF);
  pix.setPixelColor(47, 0x0000FF);
  pix.show();
  delay(50);
  // pix.setPixelColor(40, 0x000000);
  // pix.setPixelColor(33, 0x000000);
  // pix.setPixelColor(26, 0x000000);
  // pix.setPixelColor(19, 0x000000);
  // pix.setPixelColor(20, 0x000000);
  // pix.setPixelColor(29, 0x000000);
  // pix.setPixelColor(38, 0x000000);
  // pix.setPixelColor(47, 0x000000);
  // Next up, light 32, 25, 18, 11, 12, 21, 30, 39
  pix.setPixelColor(32, 0x0000FF);
  pix.setPixelColor(25, 0x0000FF);
  pix.setPixelColor(18, 0x0000FF);
  pix.setPixelColor(11, 0x0000FF);
  pix.setPixelColor(12, 0x0000FF);
  pix.setPixelColor(21, 0x0000FF);
  pix.setPixelColor(30, 0x0000FF);
  pix.setPixelColor(39, 0x0000FF);
  pix.show();
  delay(50);
  // pix.setPixelColor(32, 0x000000);
  // pix.setPixelColor(25, 0x000000);
  // pix.setPixelColor(18, 0x000000);
  // pix.setPixelColor(11, 0x000000);
  // pix.setPixelColor(12, 0x000000);
  // pix.setPixelColor(21, 0x000000);
  // pix.setPixelColor(30, 0x000000);
  // pix.setPixelColor(39, 0x000000);
  // Next up, light 24, 17, 10, 3, 4, 13, 22, 31
  pix.setPixelColor(24, 0x0000FF);
  pix.setPixelColor(17, 0x0000FF);
  pix.setPixelColor(10, 0x0000FF);
  pix.setPixelColor(3, 0x0000FF);
  pix.setPixelColor(4, 0x0000FF);
  pix.setPixelColor(13, 0x0000FF);
  pix.setPixelColor(22, 0x0000FF);
  pix.setPixelColor(31, 0x0000FF);
  pix.show();
  delay(50);
  // pix.setPixelColor(24, 0x000000);
  // pix.setPixelColor(17, 0x000000);
  // pix.setPixelColor(10, 0x000000);
  // pix.setPixelColor(3, 0x000000);
  // pix.setPixelColor(4, 0x000000);
  // pix.setPixelColor(13, 0x000000);
  // pix.setPixelColor(22, 0x000000);
  // pix.setPixelColor(31, 0x000000);
  // Next up, light 16, 9, 2, 5, 14, 23
  pix.setPixelColor(16, 0x0000FF);
  pix.setPixelColor(9, 0x0000FF);
  pix.setPixelColor(2, 0x0000FF);
  pix.setPixelColor(5, 0x0000FF);
  pix.setPixelColor(14, 0x0000FF);
  pix.setPixelColor(23, 0x0000FF);
  pix.show();
  delay(50);
  pix.clear();
  pix.show();
  #endif
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

  #ifdef BOARD_HAS_NEOPIXEL
  pix.begin();
  pix.clear();
  pix.setBrightness(25);
  #endif
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  //esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  //Serial.println("Starting");
  char Buf[50];
  char bufBSSID[64];
  char BufEnc[50];

  if ((millis() - lastTime) > timerDelay) {
    // Set values to send

    //myData.b = random(1,20);
    //myData.c = 1.2;
    int n = WiFi.scanNetworks(false, true, false, 300, boardID);
    if (n == 0) {
      Serial.println("No networks found");
      Serial.println("No networks found");
    } else {
      for (int8_t i = 0; i < n; i++) {
        //delay(10);
        if (seen_mac(WiFi.BSSID(i))) {
          Serial.println("We've already seen it");
          //BSSIDchar = WiFi.BSSID(i);
          //BSSIDchar.toCharArray(bufBSSID, 64);
          //strcpy(myData.bssid, Buf);
          //Serial.println(myData.bssid);
          Serial.println(myData.boardID);
          continue;
        }
        Serial.println("We havent seen it");
        String MacString = WiFi.BSSIDstr(i).c_str();
        //myData.bssid = MacString;
        MacString.toCharArray(bufBSSID, 64);
        strcpy(myData.bssid, bufBSSID);
        Serial.println(myData.bssid);
        //myData.bssid = WiFi.BSSID(i);
        //Serial.print("MyData.bssid: ");

        //Serial.println(myData.bssid);
        String AP = WiFi.SSID(i);
        AP.toCharArray(Buf, 50);
        strcpy(myData.ssid, Buf);
        Serial.print("SSID: ");
        Serial.println(myData.ssid);
        //String ENC = security_int_to_string(WiFi.encryptionType(i));

        //ENC.toCharArray(BufEnc, 32);
        //strcpy(myData.encryptionType, BufEnc);
        //myData.encryptionType = authtype;
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
        myData.boardID = boardID;  //YOU NEED TO CHANGE THE BOARDID TO BE UNIQUE FOR EVERY SUB BVEFORE YOU FLASH IT. DONT DO IT HERE THOUGH
        Serial.println(myData.boardID);
        save_mac(WiFi.BSSID(i));
        esp_now_send(broadcastAddress, (uint8_t*)&myData, sizeof(myData));
        #ifdef BOARD_HAS_NEOPIXEL
        play_animation();
        #endif
        // //digitalWrite(2, LOW);
        // delay(200);
        // //digitalWrite(2, HIGH);
      }



      lastTime = millis();
    }
  }

}