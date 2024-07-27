#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <SPIFFS.h>
#include <FS.h>

// Constants defined here:
#define LOG_FILE_PREFIX "/gpslog"
#define MAX_LOG_FILES 100
#define LOG_FILE_SUFFIX "csv"
#define LOG_COLUMN_COUNT 11
#define LOG_RATE 500
#define NOTE_DH2 661
File myFile;
char logFileName[13];
int totalNetworks = 0;
unsigned long lastLog = 0;

#define GPS_TX 17
#define GPS_RX 18
#define GPS_EN 4
static const uint32_t GPSBaud = 9600;   // GPS module default baud rate is 9600

const String wigleHeaderFileFormat = "WigleWifi-1.4,appRelease=2.26,model=custom,release=0.0.0,device=The_Wifydra,display=3fea5e7,board=esp32s3,brand=Espressif";
char* log_col_names[LOG_COLUMN_COUNT] = {
  "MAC", "SSID", "AuthMode", "FirstSeen", "Channel", "RSSI", "Latitude", "Longitude", "AltitudeMeters", "AccuracyMeters", "Type"
};

// #define TFT_MOSI 15
// #define TFT_CLK 16
// #define TFT_CS 5
// #define TFT_DC 6
// #define TFT_RST 8
// #define TFT_BACKLITE 9

// Adafruit_NV3030B tft = Adafruit_NV3030B(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

#define OLED_SCL 14
#define OLED_SDA 13

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);


// #define SD_MOSI 4
// #define SD_MISO 5
// #define SD_SCK 6
// #define SD_CS 7

#define fs SPIFFS
#define FORMAT_SPIFFS_IF_FAILED false

String recentSSID;
String recentSSID1;
String recentSSID2;
String board1SSID;
String board2SSID;
String board3SSID;
String board4SSID;
String board5SSID;
String board6SSID;
String board7SSID;
String board8SSID;
String board9SSID;
String board10SSID;
String board11SSID;
String board12SSID;
String board13SSID;
String board14SSID;

int board1Seen = 0;
int board2Seen = 0;
int board3Seen = 0;
int board4Seen = 0;
int board5Seen = 0;
int board6Seen = 0;
int board7Seen = 0;
int board8Seen = 0;
int board9Seen = 0;
int board10Seen = 0;
int board11Seen = 0;
int board12Seen = 0;
int board13Seen = 0;
int board14Seen = 0;

int calculateFreeStoragePercentage()
{
  Serial.println("Total SPIFFS size: " + String(fs.totalBytes()));
  Serial.println("Total SPIFFS space used: " + String(fs.usedBytes()));

  float percentage = 100.0 * (float)fs.usedBytes() / fs.totalBytes();
  Serial.println("SPIFFS used: " + String(percentage) + "%");
  return percentage;
}

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


void updateFileName() {
  int i = 0;
  for (; i < MAX_LOG_FILES; i++) {
    memset(logFileName, 0, strlen(logFileName));
    sprintf(logFileName, "%s%d.%s", LOG_FILE_PREFIX, i, LOG_FILE_SUFFIX);
    if (!fs.exists(logFileName)) {
     
     
      break;
    } else {
     
     
    }
  }
 
 
}

void printHeader() {
  File logFile = fs.open(logFileName, FILE_WRITE);
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

typedef struct struct_message {
  char bssid[64];
  char ssid[32];
  char encryptionType[16];
  int32_t channel;
  int32_t rssi;
  int boardID;
} struct_message;

// Create a struct_message called myData
struct_message myData;

HardwareSerial GPSSerial(2);
TinyGPSPlus gps;

static void smartDelay(unsigned long ms)  // custom version of delay() ensures that the gps object is being "fed".
{
  unsigned long start = millis();
  do {
    while (GPSSerial.available())
      gps.encode(GPSSerial.read());
  } while (millis() - start < ms);
}

// Callback function that will be executed when data is received
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  File logFile = fs.open(logFileName, FILE_APPEND);
  memcpy(&myData, incomingData, sizeof(myData));
 
 
 
 
  logFile.print(myData.bssid);
  logFile.print(",");
 
 
  String SSIDString = myData.ssid;
  SSIDString.replace(",", ".");  // Commas in SSID break the CSV
  logFile.print(SSIDString);
  logFile.print(",");
 
 
  logFile.print(myData.encryptionType);
  logFile.print(",");
 
  
  logFile.print(gps.date.year());
  logFile.print("-");
 
  logFile.print(pad(gps.date.month()));
  logFile.print("-");

  logFile.print(pad(gps.date.day()));
  logFile.print(" ");
 
  logFile.print(pad(gps.time.hour()));
  logFile.print(":");
 
  logFile.print(pad(gps.time.minute()));
  logFile.print(":");
 
  logFile.print(pad(gps.time.second()));
  logFile.print(",");
 
 
  logFile.print(myData.channel);
  logFile.print(",");
 
 
  logFile.print(myData.rssi);
  logFile.print(",");
 
 
  logFile.print(gps.location.lat(), 8);
  logFile.print(",");
 
 
  logFile.print(gps.location.lng(), 8);
  logFile.print(",");
 
 
  logFile.print(gps.altitude.meters());
  logFile.print(",");
 
 
  logFile.print(gps.hdop.value());
 
 
  logFile.print(",");
  logFile.print("WIFI");
  logFile.println();
  logFile.close();
  recentSSID2 = recentSSID1;
  recentSSID1 = recentSSID;
  recentSSID = myData.ssid;
  if (myData.boardID == 1) {
    board1SSID = myData.ssid;
   
   
    board1Seen += 1;
   
  }
  if (myData.boardID == 2) {
    board2SSID = myData.ssid;
   
   
    board2Seen += 1;
   
  }
  if (myData.boardID == 3) {
    board3SSID = myData.ssid;
   
   
    board3Seen += 1;
   
  }
  if (myData.boardID == 4) {
    board4SSID = myData.ssid;
   
   
    board4Seen += 1;
   
  }
  if (myData.boardID == 5) {
    board5SSID = myData.ssid;
   
   
    board5Seen += 1;
   
  }
  if (myData.boardID == 6) {
    board6SSID = myData.ssid;
   
   
    board6Seen += 1;
   
  }
  if (myData.boardID == 7) {
    board7SSID = myData.ssid;
   
   
    board7Seen += 1;
   
  }
  if (myData.boardID == 8) {
    board8SSID = myData.ssid;
   
   
    board8Seen += 1;
   
  }
  if (myData.boardID == 9) {
    board9SSID = myData.ssid;
   
   
    board9Seen += 1;
   
  }
  if (myData.boardID == 10) {
    board10SSID = myData.ssid;
   
   
    board10Seen += 1;
   
  }
  if (myData.boardID == 11) {
    board11SSID = myData.ssid;
   
   
    board11Seen += 1;
   
  }
  if (myData.boardID == 12) {
    board12SSID = myData.ssid;
   
   
    board12Seen += 1;
   
  }
  if (myData.boardID == 13) {
    board13SSID = myData.ssid;
   
   
    board13Seen += 1;
   
  }
  if (myData.boardID == 14) {
    board14SSID = myData.ssid;
   
   
    board14Seen += 1;
   
  }
 
 totalNetworks +=1;



 
 
 
}

void setup() {
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  pinMode(GPS_EN, OUTPUT);
  digitalWrite(GPS_EN, HIGH);
  GPSSerial.begin(GPSBaud, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.begin(115200);
  long time = millis();

  while(!Serial || millis() - time < 10000) 
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  if(Serial)
  {
    Serial.println("Serial connected, dumping SPIFFS contents:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file)
    {
      Serial.print("  FILE: ");
      Serial.println(file.name());
      String contents = file.readString();
      Serial.println(contents);
      file.close();
      file = root.openNextFile();
    }
    time = millis();
    calculateFreeStoragePercentage();
    Serial.println("Want to format SPIFFS? Press 'F' within 5 seconds to format.");
    while(millis() - time < 5000)
    {
      
      if(Serial.available())
      {
        char c = Serial.read();
        if(c == 'F')
        {
          Serial.println("Are you sure? Press 'Y' to format.");
          while(!Serial.available())
          {
            delay(10);
          }
          c = Serial.read();
          if(c == 'Y')
          {
            SPIFFS.format();
            Serial.println("SPIFFS formatted.");
            break;
          }
        }
      }
    }

  }

  u8g2.begin();
  u8g2.setFont(u8g2_font_t0_14b_tr);
  u8g2.clearBuffer();
  u8g2.setCursor(0, 20);
  u8g2.print("Hello World!");
  u8g2.sendBuffer();
  

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
   
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  updateFileName();
  printHeader();
  void displayinfo();
}





// void displayinfo() {
//   tft.fillScreen(NV3030B_BLACK);
//   tft.setCursor(0, 60);
//   tft.setTextWrap(true);
//   tft.setTextSize(1);
//   tft.print("LT ");
//   tft.print(gps.location.lat(), 1);
//   //Serial.print(gps.location.lat());
//   tft.print(" LN");
//   tft.print(gps.location.lng(), 1);
//   //Serial.println(gps.location.lng);
//   tft.print(" S");
//   tft.println(gps.satellites.value());

//   tft.print("1:");
//   tft.print(board1Seen);
//   tft.print(" | ");
//   tft.print("2:");
//   tft.println(board2Seen);
  

//   tft.print("3:");
//   tft.print(board3Seen);
//   tft.print(" | ");
//   tft.print("4:");
//   tft.println(board4Seen);
  
  
//   tft.print("5:");
//   tft.print(board5Seen);
//   tft.print(" | ");
//   tft.print("6:");
//   tft.println(board6Seen);

  
//   tft.print("7:");
//   tft.print(board7Seen);
//   tft.print(" | ");
//   tft.print("8:");
//   tft.println(board8Seen);

  
//   tft.print("9:");
//   tft.print(board9Seen);
//   tft.print(" | ");
//   tft.print("10:");
//   tft.println(board10Seen);
  

//   tft.print("11:");
//   tft.print(board11Seen);
//   tft.print(" | ");
//   tft.print("12:");
//   tft.println(board12Seen);

//   tft.print("13:");
//   tft.print(board13Seen);
//   tft.print(" | ");
//   tft.print("14:");
//   tft.println(board14Seen);







//   tft.print(recentSSID);
//   tft.print(", ");
//   tft.print(recentSSID1);
//   tft.print(", ");
//   tft.println(recentSSID2);
//   tft.println();
//   tft.print("Nets: ");
//   tft.print(totalNetworks);
  
// /**
//   tft.println("");
//   tft.print(board1SSID);
//   tft.print(", ");
//   tft.print(board2SSID);
//   tft.print(", ");
//   tft.print(board3SSID);
//   tft.print(", ");
//   tft.print(board4SSID);
//   tft.print(", ");
//   tft.print(board5SSID);
//   tft.print(", ");
//   tft.print(board6SSID);
//   tft.print(", ");
//   tft.print(board7SSID);
//   tft.print(", ");

// */

//   /*


//   tft.update();
//   delay(10);
// */  

void displayinfo() {
  u8g2.clearBuffer();
  u8g2.setCursor(0, 10);
  u8g2.print("LT ");
  u8g2.print(gps.location.lat(), 1);
  //Serial.print(gps.location.lat());
  u8g2.print(" LN");
  u8g2.print(gps.location.lng(), 1);
  //Serial.println(gps.location.lng);
  u8g2.print(" S");
  u8g2.print(gps.satellites.value());

  u8g2.setCursor(0, 20);

  u8g2.print("1:");
  u8g2.print(board1Seen);
  u8g2.print(" | ");
  u8g2.print("2:");
  u8g2.print(board2Seen);
  
  u8g2.setCursor(0, 30);

  u8g2.print("3:");
  u8g2.print(board3Seen);
  u8g2.print(" | ");
  u8g2.print("4:");
  u8g2.print(board4Seen);

  u8g2.setCursor(0, 40);
  
  
  u8g2.print("5:");
  u8g2.print(board5Seen);
  u8g2.print(" | ");
  u8g2.print("6:");
  u8g2.print(board6Seen);

  u8g2.setCursor(0, 50);

  
  u8g2.print("7:");
  u8g2.print(board7Seen);
  u8g2.print(" | ");
  u8g2.print("8:");
  u8g2.print(board8Seen);

  u8g2.setCursor(0, 60);

  
  u8g2.print("9:");
  u8g2.print(board9Seen);
  u8g2.print(" | ");
  u8g2.print("10:");
  u8g2.print(board10Seen);

  u8g2.setCursor(0, 70);
  

  u8g2.print("11:");
  u8g2.print(board11Seen);
  u8g2.print(" | ");
  u8g2.print("12:");
  u8g2.print(board12Seen);

  u8g2.setCursor(0, 80);

  u8g2.print("13:");
  u8g2.print(board13Seen);
  u8g2.print(" | ");
  u8g2.print("14:");
  u8g2.print(board14Seen);

  u8g2.setCursor(0, 90);

  u8g2.print(recentSSID);
  u8g2.print(", ");
  u8g2.print(recentSSID1);
  u8g2.print(", ");
  u8g2.print(recentSSID2);
  u8g2.print();

  u8g2.setCursor(0, 100);
  u8g2.print("Nets: ");
  u8g2.print(totalNetworks);

  u8g2.sendBuffer();


}
void loop() {
  //displayinfo();   // run procedure displayinfo
  smartDelay(5000);  // run procedure smartDelay
  displayinfo();  
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    // tft.setCursor(0, 60);
    // tft.println("No GPS detected:");
    // tft.println(" check wiring.");

    u8g2.clearBuffer();
    u8g2.setCursor(0, 20);
    u8g2.print("No GPS detected:");
    u8g2.print(" check wiring.");
    u8g2.sendBuffer();

    
  }
}
