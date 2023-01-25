/*********
ESP32 보드의 와이파이 접속 SSID와 PW를 휴대폰을 이용하여 설정하는 방법
WIFI SSID와 PW 입력하는 방법
1.휴대폰 wifi로 ESP-WIFI-MANAGER에 접속
2.크롬, 웨일, 사파리 등 인터넷 창에 192.168.4.1 를 입력하여 접속
3.접속한 화면에서 SSID , pw, thingspeak 채널 번호, thingspeak 채널 api를 입력 후 제출
4.휴대폰에서 ESP-WIFI-MANAGER 와이파이가 접속이 끊어졌는지 확인
5.esp32 보드에 와이파이가 접속이 되어 정상적으로 작동하는 지 확인

아래의 코드가 정상 작동하기 위한 방법 
https://blog.naver.com/PostView.naver?blogId=mapes_khkim&logNo=221854594635&parentCategoryNo=1&categoryNo=&viewDate=&isShowPopularPosts=false&from=postView
위의 링크로 들어가 SPIFFS 설정하기 설정을 하면 상단 tools 중에서 
wifi 101 ~~
[ ====================== ]
board : "ESP32 DEV MODULE"
저 둘 사이에 esp32 sketch data upload 라는 창이 생성됨
C:\Users\Dell\Documents\Arduino\AM1008W-wifi-connect-V03\data
아두이노 코드 파일이 있는 곳에 data라고 폴더를 생성한 후 
필요한 파일을 추가하기.
*/
//============ wifi connect
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
int wifi_connected_history = 0;
// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_5 = "thingspeak_ch_number";
const char* PARAM_INPUT_6 = "thingspeak_ch_api";

//Variables to save values from HTML form
String ssid;
String pass;
String thingspeak_ch_number;
String thingspeak_ch_api;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ch_numberPath = "/ch_num.txt";
const char* apiPath = "/api.txt";

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

//=======================================
//===========sensor

#include <am1008w_k_i2c.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "FS.h"
#include <SD.h>
#include <ThingSpeak.h>
// sd card
SPIClass SDSPI(HSPI);
#define SD_CS       33      // 핀모드 설정
#define SPI_SCK     25      // 핀모드 설정
#define SPI_MISO     27     // 핀모드 설정
#define SPI_MOSI     26     // 핀모드 설정
long timezone = 1; 
byte daysavetime = 1;
File  root;                // SD 카드 
struct tm tmstruct ;
String filename01 = "/" ;
String filename02 ;
String filename03 = ".txt" ;
String filename ;
String YEAR;
String MONTH;
String DAY;
int button_num = 0;
int first_time = 0;
int a = 0;
//display and sensor
AM1008W_K_I2C am1008w_k_i2c;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);
unsigned long previousMillis_sensor = 0;
unsigned long previousMillis_sensor1= 0;
const long interval_sensor=1000;
const long interval_sensor1=60000;
WiFiClient  client;
//=====================================


// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" ){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }
  return true;
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(0,INPUT);             //왼쪽 버튼
  pinMode(35,INPUT);            //오른쪽 버튼
  //sensor===
  am1008w_k_i2c.begin();   

  tft.init();
  tft.setRotation(1);
  img.setColorDepth(8);
  img.createSprite(240, 320);
  img.fillScreen(TFT_BLACK);
  SPI.begin(25,27,26,33); //spi 읽는 루트 SCK, MISO, MOSI, CS 순서 
  Wire.begin();
  
  //========= SPIFFS에 입력한 wifi ssid와 pw를 저장함.
  initSPIFFS();
  
  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  thingspeak_ch_number = readFile(SPIFFS, ch_numberPath);
  thingspeak_ch_api = readFile(SPIFFS, apiPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(thingspeak_ch_number);
  Serial.println(thingspeak_ch_api);  

  if(initWiFi()) {
    wifi_connected_history = 1;
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
          if (p->name() == PARAM_INPUT_5) {
            thingspeak_ch_number = p->value().c_str();
            Serial.print("thingspeak_ch_number set to: ");
            Serial.println(thingspeak_ch_number);
            // Write file to save value
            writeFile(SPIFFS, ch_numberPath, thingspeak_ch_number.c_str());
          }
          if (p->name() == PARAM_INPUT_6) {
            thingspeak_ch_api = p->value().c_str();
            Serial.print("thingspeak_ch_api set to: ");
            Serial.println(thingspeak_ch_api);
            // Write file to save value
            writeFile(SPIFFS, apiPath, thingspeak_ch_api.c_str());
          }
        }
      }
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
  if (WiFi.status() == WL_CONNECTED){
    ThingSpeak.begin(client);
  }
}

void button() {
  if (digitalRead(0) ==0){   // 상단 TTGO 글자를 기준으로 왼쪽 버튼 눌렀을 때 실행
    button_num =1;
    writeFile(SPIFFS, ssidPath,"");
    writeFile(SPIFFS, passPath,"");
    writeFile(SPIFFS, ch_numberPath,"");
    writeFile(SPIFFS, apiPath,"");
  }
}
// SD_SAVE
void SD_SAVE(){
  configTime(3600*timezone, daysavetime*3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");   // 시간 설정 (예제 SD_TEST 발췌)
    tmstruct.tm_year = 0;
    getLocalTime(&tmstruct, 5000);
    YEAR = (tmstruct.tm_year)+1900;
    MONTH = ( tmstruct.tm_mon)+1;
    DAY =  tmstruct.tm_mday;
    filename02 =  YEAR + MONTH + DAY;
    filename = filename01 + filename02 + filename03;    
    SD.begin(33,SPI);  // SD카드 읽기 시작
    root = SD.open(filename,FILE_APPEND);            // SD카드 데이터 저장 방식 WRITE는 기존 데이터가 삭제되고 작성되어서 APPEND를 이용하여 추가
    root.print("Temp,");
    root.print(am1008w_k_i2c.get_temperature());
    root.print(",Humi,");
    root.print(am1008w_k_i2c.get_humidity());
    root.print(",VOCs,");
    root.print(am1008w_k_i2c.get_voc());
    root.print(",PM1.0,");
    root.print(am1008w_k_i2c.get_pm1p0());
    root.print(",PM2.5,");
    root.print(am1008w_k_i2c.get_pm2p5());
    root.print(",PM10,");
    root.print(am1008w_k_i2c.get_pm10());
    root.print(",CO2,");
    root.print(am1008w_k_i2c.get_co2());
    
    root.printf(",%d-%02d-%02d %02d:%02d:%02d\n",(tmstruct.tm_year)+1900,( tmstruct.tm_mon)+1, tmstruct.tm_mday,(tmstruct.tm_hour)+7 , tmstruct.tm_min, tmstruct.tm_sec);
    root.flush();                                        // 바로 윗 줄이 데이터 시간 기록 
    root.close();
}

void ThingSpeak_()
{
  unsigned long currentMillis=millis();
  if(tmstruct.tm_min != a){
    a= tmstruct.tm_min;
      ThingSpeak.setField(1, am1008w_k_i2c.get_temperature());
      ThingSpeak.setField(2, am1008w_k_i2c.get_humidity());
      ThingSpeak.setField(3, am1008w_k_i2c.get_voc());
      ThingSpeak.setField(4, am1008w_k_i2c.get_pm1p0());
      ThingSpeak.setField(5, am1008w_k_i2c.get_pm2p5());
      ThingSpeak.setField(6, am1008w_k_i2c.get_pm10());
      ThingSpeak.setField(7, am1008w_k_i2c.get_co2());
      unsigned long ch_num = strtoul(thingspeak_ch_number.c_str(),NULL,10);
      ThingSpeak.writeFields(ch_num, thingspeak_ch_api.c_str());
      Serial.println(ch_num);
      Serial.println(thingspeak_ch_api.c_str());
   }
}
void loop() {
  if(WiFi.status() == WL_CONNECTED && button_num == 0){
 uint8_t ret = am1008w_k_i2c.read_data_command();   // 이부분이 진행이 안되는지

    img.fillSprite(TFT_BLACK);
    img.setTextColor(TFT_WHITE);
    img.setFreeFont(&FreeSans9pt7b);
    img.setCursor (0, 18);
    img.print ("Temperature");
    img.setCursor (120, 18);
    img.print ("Humidity");
    img.setCursor (120, 76);
    img.print("VOC");
    img.setCursor (0, 76);
    img.print ("PM1.0");
    img.setCursor (0, 102);
    img.print ("PM2.5");
    img.setCursor (0, 128);
    img.print ("PM10");
    img.setCursor (120, 102);
    img.print("CO2");
    if (first_time ==0){
      a= tmstruct.tm_min;
      first_time =1;
    }    

        
  if (ret == 0) {                            
    img.setTextColor(TFT_YELLOW);
    img.setTextSize(1); 
    img.setFreeFont(&FreeSans18pt7b);    
    img.setCursor (0, 50);
    img.print(am1008w_k_i2c.get_temperature());
    img.setFreeFont(&FreeSans9pt7b); 
    img.print (" º""C");
    img.setCursor (120, 50);
    img.setFreeFont(&FreeSans18pt7b); 
    img.print(am1008w_k_i2c.get_humidity());
    img.setFreeFont(&FreeSans9pt7b); 
    img.print (" %");
    img.setTextColor(TFT_YELLOW);
    img.setTextSize(0.5); 
    img.setFreeFont(&FreeSans9pt7b);  
    img.setCursor (60, 76);
    img.print(am1008w_k_i2c.get_pm1p0());
    img.setCursor (60, 102);
    img.print(am1008w_k_i2c.get_pm2p5());
    img.setCursor (60, 128);
    img.print(am1008w_k_i2c.get_pm10());
    img.setCursor (170, 102);
    img.print(am1008w_k_i2c.get_co2());
    img.setCursor (170, 76);
    img.print(am1008w_k_i2c.get_voc());
    img.setCursor (120,128);
    img.setTextColor(TFT_RED);
    img.setFreeFont(&FreeSans9pt7b);
    img.print("wifi reset >>>");
    SD_SAVE();
    ThingSpeak_();
  }
  delay(1000);
  } else {
    img.fillSprite(TFT_BLACK);
    img.setTextColor(TFT_WHITE);
    img.setFreeFont(&FreeSans9pt7b);
    img.setCursor(0, 18);
    img.print("connect wifi");
    img.setCursor(0,48);
    img.print("[ESP-WIFI-MANAGER]");
    img.setCursor(0,78);
    img.setTextColor(TFT_YELLOW);
    img.print("Access 192.168.4.1");
    img.setTextColor(TFT_GREEN);
    img.setCursor(0,108);
    img.print("Enter SSID and PW");
  }
  if(WiFi.status() != WL_CONNECTED && wifi_connected_history ==1){
    img.fillSprite(TFT_BLACK);
    img.setTextColor(TFT_WHITE);
    img.setFreeFont(&FreeSans9pt7b);
    img.setCursor(0,18);
    img.print("wifi unconnected");
    img.setCursor(0,48);
    img.setTextColor(TFT_YELLOW);
    img.print("check wifi host");
    img.setTextColor(TFT_RED);
    img.setCursor(0,78);
    img.print("restart sensor or");
    img.setCursor(0,108);
    img.print("reset wifi SSID / PW");
    img.setCursor(120,128);
    img.print("wifi reset >>>");
    first_time = 0;
  }
  if (button_num == 1){
    img.fillSprite(TFT_BLACK);
    img.setTextColor(TFT_RED);
    img.setTextSize(1);
    img.setFreeFont(&FreeSans9pt7b);
    img.setCursor (0, 18);
    img.print("WifI reset starting...");
    img.setTextColor(TFT_WHITE);
    img.setCursor(0,48);
    img.print("restart after 5 seconds");
    delay(5000);
    ESP.restart();
  }
  button();
  img.pushSprite(0,0);
}
