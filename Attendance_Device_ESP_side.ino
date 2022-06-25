#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <UnixTime.h>
#include <pcmConfig.h>
#include <pcmRF.h>
#include <TMRpcm.h>
#include "db_methods.h"
#include "network_data.h"
#include "Config.h"
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "images.h"
#include "Font.h"

#define OLED_MAIN_FRAME 0
#define OLED_CONECTING_GIF_1 1
#define OLED_CONNECTING_GIF_2 2
#define OLED_CONNECTING_GIF_3 3
#define OLED_CONNECTING_GIF_4 4
#define OLED_CONNECTED_FRAME 5
#define OLED_WELLCOME_FRAME 6
#define OLED_WAIT_4_ADD 7
#define OLED_STUDENT_ADDED 8
#define OLED_ATTENDANCE_ENDED 9

#define BELLA_CIAO "Bella.wav"

#define BLOCKA_ADDRES 2
#define TRAILER_BLOCK 3
#define RFID_SS_PIN 2
#define RST_PIN 0
#define SD_CARD_SS_PIN 16
#define IRAN_UTC_OFFSET 4.5
#define DIRECTOR_PATH_NAME "/db/"
#define SELECTED_NTP_SERVER "ntp.day.ir"
/*
 "ntp.day.ir"
 "pool.ntp.org"
 "pool.ntp.org"
 "pool.ntp.org"
 "pool.ntp.org"
*/

const long utcOffsetInSeconds = 3600 * IRAN_UTC_OFFSET;
const int UTC_OFFSET =  4.5;

uint64_t millis_var = 0;
bool visible_overlay = true;

//Week Days
const String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
const String months[12]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};


LinkedList <Student*> students = LinkedList <Student*>();
LinkedList <Student*> present_list = LinkedList <Student*>();
String present_str = "error";

const IPAddress local_IP(192, 168, 43, 100); // Static IP
// const IPAddress local_IP(192, 168, 146, 150); // Static IP
const IPAddress gateway (192, 168, 43, 1);   // GatewayIP
// const IPAddress gateway (192, 168, 146, 131);   // GatewayIP
const IPAddress subnet  (255, 255, 255, 0);  // Subnet MASK
const IPAddress DNS1(8, 8, 8, 8); //dns 1
const IPAddress DNS2(8, 8, 8, 8); //dns 2

// Create a WebServer object on port 80
ESP8266WebServer server(80);
// Create a MFRC522 RFID reader object 
MFRC522 mfrc522(RFID_SS_PIN, RST_PIN);
// Create a Configuration object. it's configs and settings data handeler
Config conf(0);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

UnixTime date_time(0);

TMRpcm audio;

SSD1306Wire display(0x3c, D2, D1); //setup OLED 
OLEDDisplayUi ui(&display );  //User Interface Defenition

//prototype
void drawFrame(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y);
void drawWiFiConnecting0(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y);
void drawWiFiConnecting1(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y);
void drawWiFiConnecting2(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y);
void drawWiFiConnecting3(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y);
void drawWiFiConnected(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y);
void drawWellCome(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y);
void drawWait4Add(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y);
void drawStudentAdded(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y);
void drawAttendanceEnded(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y);
void drawOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);

#define NUMBER_OF_FRAMES 10
FrameCallback frames[] = {drawFrame, drawWiFiConnecting0, drawWiFiConnecting1, drawWiFiConnecting2, drawWiFiConnecting3, drawWiFiConnected, drawWellCome, drawWait4Add, drawStudentAdded, drawAttendanceEnded};  //Frames Calling

#define NUMBER_OF_OVERLAYS 1
OverlayCallback overlays[] = {drawOverlay};  // Overlays Calling

void setup() {
  audio.speakerPin = 15;
  // conf.init();
  Serial.begin(115200);
  Serial.println("SPI begining...");

  ui.init(); //Initialize the User Interface
  display.clear();
  ui.setTargetFPS(30); //Setup UI Frames
  ui.disableAllIndicators(); //Make indicators Off
  ui.disableAutoTransition(); //Disable auto Transition
  ui.setFrames(frames, NUMBER_OF_FRAMES);  // SetFrames to show
  // ui.setTimePerTransition(500);
  display.flipScreenVertically();

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  conf.loadConfigurations(DEFAULT_CONFIG_FILE_NAME);
  Serial.println(printContentFileFromSPIFFS(DEFAULT_CONFIG_FILE_NAME));

  SPI.begin();
  mfrc522.PCD_Init();
  delay(4); // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  // pinMode(SD_CARD_SS_PIN, OUTPUT);
  // digitalWrite(SD_CARD_SS_PIN, 1);
  // delay(1);
  if (!SD.begin(SD_CARD_SS_PIN)) 
    Serial.println("SD init failed!");
  Serial.println("SD init done.");

  if (!SD.exists("/db")) {
    Serial.println("db folder is not exist");
    if(SD.mkdir("db"))
      Serial.println("db folder made");
  }

  audio.play(BELLA_CIAO);

  connectToNetwork();
  
  server.begin();

  // server.on("/", handleRoot);
  server.on("/add", addStudent);
  server.on("/present", handlePresent);
  server.on("/settings", handleSettings);
  server.on("/reset-factory", resetFactory);
  server.on("/set-manual-attendance", manualAttendance);
//  SPIFFS.format();

  load(&students);
  if(students.size() > 0) 
    students.remove(0);

  Student::showAllStudent(&students);

  initCorrectTime();

  ui.setOverlays(overlays, NUMBER_OF_OVERLAYS);  // SetOverlays to show
}

void loop() {
  updateDateTime();

  server.handleClient();

  if(isPresentTime()) {
    if(isNewCard()) {
      Serial.println("Card detected!");
      String nc = readNationalCode();
      Serial.println(nc);
      int i = isInStudentsList(nc);
      if(isInPresentList(nc) == -1 && i != -1) {
        Serial.print("Student is present: ");
        Serial.println(students.get(i)->getNationalCode());
        present_list.add(new Student(students.get(i)->getFirstName(), students.get(i)->getLastName(), students.get(i)->getNumberPhone(), students.get(i)->getNationalCode()));
        present_list.get(present_list.size()-1)->setTimeToCome(timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
        present_str = savePresentToSD(DIRECTOR_PATH_NAME+getFormatedDate()+".txt", &present_list);
        Serial.println("Presetnt tine ended and preset list saved now");
        ui.switchToFrame(OLED_WELLCOME_FRAME);
        ui.update();
      }
      millis_var = millis();
    }
  }

  if(millis()-millis_var > 3000) {
    ui.switchToFrame(OLED_MAIN_FRAME);
    ui.update();
  }
  
  display.flipScreenVertically();
  ui.update();
}

bool connectToNetwork() {
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet, DNS1, DNS2)) 
    Serial.println("WiFi config failed");
    
  Serial.println("Connecting to AccessPoint");
  WiFi.begin(conf.ssid, conf.password);
  uint8_t i = 1;
  while (WiFi.status() != WL_CONNECTED){
    ui.switchToFrame(i);
    ui.update();
    i = (i >= 4)? 1:i+1;
    Serial.print(".");
    delay(350);
  }
  ui.switchToFrame(OLED_CONNECTED_FRAME);
  ui.update();
  Serial.println("WiFi Connected!!");
  Serial.println("\nIP: "+WiFi.localIP().toString());
  Serial.println("GatewayIP: "+WiFi.gatewayIP().toString());
  Serial.println("Hostname: "+WiFi.hostname());
  delay(2500);
  ui.switchToFrame(OLED_MAIN_FRAME);
  ui.update();
}

void addStudent() {
  // Serial.println("adding student");
  String Fname = server.arg("FName");
  String Lname = server.arg("LName");
  String number_phone = server.arg("Number");
  String national_code = server.arg("Code");
  // Serial.println("inpormation recived");
  visible_overlay = false;
  // Serial.println("visible_overlay");
  // Serial.println(visible_overlay);
  // Serial.println("Waiting for Card or fingerprint");
  delay(1000);
  waitForCard();
  // Serial.println("card is present");
  writeNationalCode(national_code);
  // Serial.println("natinonal code wrote on card");
  students.add(new Student(Fname, Lname, number_phone, national_code));
  // Serial.println("student added to list");
  save(&students);
  // Serial.println("students saved on SPIFFS");
  server.send(200, "text/plain", "Student added!");
  // Serial.println("added respons sent");
  Student::showAllStudent(&students);
  ui.switchToFrame(OLED_STUDENT_ADDED);
  ui.update();
  // Serial.println("Student adding complited");
  delay(3500);
  visible_overlay = true;
  ui.switchToFrame(OLED_MAIN_FRAME);
  ui.update();
  // Serial.println("back to normal work");
}

void handlePresent() {
  // String presents = pack_string_property(&present_list, StudentProperty::NATIONAL_CODE, '*');
  String requesteDate = server.arg("date");
  Serial.print("Requested date: ");
  Serial.println(requesteDate);
  Serial.print("My date: ");
  Serial.println(getFormatedDate());
  Serial.println("Present data: ");
  Serial.println(present_str);
  if(requesteDate == getFormatedDate()) {
    server.send(200, "text/plain", present_str);
    Serial.println("today student sended");
    return;
  }

  Serial.println("Request an other day present list");

  requesteDate = DIRECTOR_PATH_NAME+requesteDate+".txt";
  Serial.println(requesteDate);
  String str_present;
  if (SD.exists(requesteDate)) {
    Serial.println("requesteDate is exists");
    File file = SD.open(requesteDate, FILE_READ);
    if (!file) {
      Serial.print("error opening ");
      Serial.println(requesteDate);
      str_present = "error";
    }
    str_present = file.readString();
    Serial.println(str_present);
    file.close();
  }
  else str_present = "error";
  server.send(200, "text/plain", str_present);
  Serial.println(str_present);
}

void handleSettings() {
  Serial.println("request on handleSettings");

  String ssid_str = server.arg("network_name");
  String password_str = server.arg("network_password");
  ssid_str.toCharArray(conf.ssid, sizeof(ssid_str));
  password_str.toCharArray(conf.password, sizeof(password_str));

  conf.access_password    = server.arg("settings_password");
  conf.email_address      = server.arg("admin_Email");
  conf.start_time.hour    = server.arg("start_hour").toInt();
  conf.start_time.minute  = server.arg("start_min").toInt();
  conf.end_time.hour      = server.arg("end_hour").toInt();
  conf.end_time.minute    = server.arg("end_min").toInt();
  conf.storage_time       = server.arg("storage_time").toInt();
  conf.send_email         = (server.arg("network_name")       == "true")? true:false;
  conf.ability_of_blind   = (server.arg("ability_of_blind")   == "true")? true:false;
  conf.call_to_parent     = (server.arg("call_to_parents")    == "true")? true:false;
  conf.massage_to_parent  = (server.arg("message_to_parents") == "true")? true:false;
  conf.start_time.seconds = 0;
  conf.end_time.seconds   = 0;

  Serial.println("recived the args");
  Serial.println(conf.access_password);
  Serial.println(conf.email_address);
  Serial.println(conf.ssid);
  Serial.println(conf.password);
  Serial.println(conf.call_to_parent);
  Serial.println(conf.end_time.hour);

  conf.saveConfigurations(DEFAULT_CONFIG_FILE_NAME);
  server.send(200, "text/plain", "Settings saved!");
  Serial.println("responed ok and settings saved!");
}

void resetFactory() {
  server.send(200, "text/plain", "Reseted Factory");
  Serial.println("Reseted Factory...");
  SPIFFS.format();
  ESP.restart();
}

void manualAttendance() {
  conf.isManualStarted = (server.arg("is_manual_started") == "true")? true:false;
  if(conf.isManualStarted)
    server.send(200, "text/plain", "Attendance began");
  else
    server.send(200, "text/plain", "Attendance is over");
}

void initCorrectTime() {
  timeClient.begin();
  timeClient.update();
  if(timeClient.getEpochTime() < 100000L)
    ESP.restart();
}

void updateDateTime() {
  timeClient.update();
  date_time.getDateTime(timeClient.getEpochTime());
}

/*
  Version1: [error]
  2022-1-24

  Version2: [problem: not unique]
  2022-1-24 --> 2022124
  2022-12-4 --> 2022124
  shortest : 2022-1-1 --> 202211   (len=6)
  bigest : 2022-11-11 --> 20221111 (len=8)

  Version3: [best and complete]
  whit out 2000:
  shortest : 2022-1-1 --> 2211   (len=4)
  bigest : 2022-11-11 --> 221111 (len=6)
  + saparator: 
  shortest : 2022-1-1 --> 22Y1M1D   (len=6)
  bigest : 2022-11-11 --> 22Y11M11D (len=8)
*/
String getFormatedDate() {
  return String(date_time.year-2000)+"Y"+String(date_time.month)+"M"+String(date_time.day);
}

String getDate4Overlay() {
  return String(date_time.day)+" "+months[date_time.month-1];
}

String getFormatedTime() {
  return ((date_time.hour > 9)? String(date_time.hour)  :"0"+String(date_time.hour))+":"
        +((date_time.minute> 9)? String(date_time.minute):"0"+String(date_time.minute));
}

int16_t isInPresentList(String ncode) { // get national code and return true if the onwner of the ncode be in the present list
  for(uint16_t i=0; i<present_list.size(); i++) 
    if(present_list.get(i)->getNationalCode() == ncode)
      return i;
  return -1;
}

int16_t isInStudentsList(String ncode) { // get national code and return true if the onwner of the ncode be in the students list
  for(uint16_t i=0; i<students.size(); i++) 
    if(students.get(i)->getNationalCode() == ncode)
      return i;
  return -1;
}

void finishCard() {
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void waitForCard() {
  while (!mfrc522.PICC_IsNewCardPresent()) {
    ui.switchToFrame(OLED_WAIT_4_ADD);
    display.flipScreenVertically();
    ui.update();
  };
  while (!mfrc522.PICC_ReadCardSerial()) {
    ui.switchToFrame(OLED_WAIT_4_ADD);
    display.flipScreenVertically();
    ui.update();
  };
}

bool isNewCard() {
  return (mfrc522.PICC_IsNewCardPresent()) && (mfrc522.PICC_ReadCardSerial());
}

String arrayToString(byte *buffer, uint8_t size) {
  String out = "";
  for(int i=0; i<size; i++) 
    out += String(buffer[i]);
  return out;
}

void stringToArray(String str, byte *buffer) {
  for(int i=0; i<str.length(); i++) {
    buffer[i] = str.charAt(i)-48;
  }
}
// read the national code from RFID tag
String readNationalCode() { 
  MFRC522::MIFARE_Key key;
  MFRC522::StatusCode status;
  byte blockAddr = 2;
  byte trailerBlock = 3;
  byte buffer[18];
  byte size = sizeof(buffer);
  for (byte i=0; i<6; i++) key.keyByte[i] = 0xFF;

  status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));

  if (status == MFRC522::STATUS_OK) { // Authenticating Is Successful
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr , buffer , &size); //Read Data From The Block
    finishCard();
    return (status == MFRC522::STATUS_OK)? arrayToString(buffer, 10):"Reading Failed";
  }
  else return "Authenticating Failed";

}
// write to the national code from RFID tag
bool writeNationalCode(String national_code) {
  MFRC522::MIFARE_Key key;
  MFRC522::StatusCode status;
  byte blockAddr = 2;
  byte trailerBlock = 3;
  byte dataBlock[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  for (byte i=0; i<6; i++) key.keyByte[i] = 0xFF;

  stringToArray(national_code, dataBlock);

  status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid)); //Authenticate using Key B

  if (status == MFRC522::STATUS_OK) { // Authenticating Using Key B Is Successful
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr , dataBlock, 16); //Write data to block
    finishCard();

    return (status == MFRC522::STATUS_OK)? true:false;
  }
}

bool isPresentTime() {
  return conf.isManualStarted || 
  ((timeClient.getHours() > conf.start_time.hour || (timeClient.getHours() == conf.start_time.hour && timeClient.getMinutes() >= conf.start_time.minute)) 
  && (timeClient.getHours() < conf.end_time.hour || (timeClient.getHours() == conf.end_time.hour && timeClient.getMinutes() <= conf.end_time.minute)));
  return true;
}

void drawFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(Irish_Grover_Regular_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(15+x, 20+y , "Absolute");
  display->drawString(20+x, 42+y , "Darkess");
}
void drawWiFiConnecting0(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x + 39, y + 0, WiFi_Connecting_0_width, WiFi_Connecting_0_height, WiFi_Connecting_0);
}
void drawWiFiConnecting1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x + 39, y + 0, WiFi_Connecting_1_width, WiFi_Connecting_1_height, WiFi_Connecting_1);
}
void drawWiFiConnecting2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x + 39, y + 0, WiFi_Connecting_2_width, WiFi_Connecting_2_height, WiFi_Connecting_2);
}
void drawWiFiConnecting3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x + 39, y + 0, WiFi_Connecting_3_width, WiFi_Connecting_3_height, WiFi_Connecting_3);
}
void drawWiFiConnected(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y){
  display->drawXbm(x + 39, y + 0, WiFi_Connecting_3_width, WiFi_Connecting_3_height, WiFi_Connecting_3);
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(25, 48, "Connected!");
}
void drawWellCome(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y) {
  display->drawXbm(x + 70, y + 16, smile_width, smile_height, smile);
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(10, 25, "WELL" );
  display->drawString(10, 40, "COME!");
}
void drawWait4Add(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y) {
  display->drawXbm(x + 0, y + 16, fingerprint_width, fingerprint_height, fingerprint);
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(50, 40, "or");
  display->drawXbm(x + 70, y + 20, RFID_Card_width, RFID_Card_height, RFID_Card);

  display->setFont(Serif_bold_13);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 0, "Waiting for..." );
}
void drawStudentAdded(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y) {
  display->drawXbm(x + 42, y + 16, student_added_width, student_added_height, student_added);

  display->setFont(Serif_bold_13);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 0, "Student added");
}
void drawAttendanceEnded(OLEDDisplay *display, OLEDDisplayUiState* state,int16_t x, int16_t y) {
  display->drawXbm(x + 42, y + 16, AttendanceTime_Ended_width, AttendanceTime_Ended_height, AttendanceTime_Ended);

  display->setFont(Serif_bold_13);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 16, "End" );
}

void drawOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  if(visible_overlay) {
    display->setFont(Serif_bold_13);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0, 0, getDate4Overlay());

    display->setFont(DSEG7_Classic_Mini_Bold_14);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(75, 0, getFormatedTime());
  }
}

void say() {

}

bool print_file_data(String path) {
  if(SPIFFS.exists(path)){
    fs::File name_file = SPIFFS.open(path, "r");
    if(!name_file){
      Serial.println("Failed to open name_file for reading");
      return false;
    }
    else{
      while (!name_file.available());
      Serial.println(name_file.readString());
      name_file.close();
      return true;
    }
  }
}

bool printContentFileFromSPIFFS(String path) {
  if(SPIFFS.exists(path)){
    // file is exist
    fs::File file = SPIFFS.open(path, "r");
    if(!file){ 
      Serial.println("Failed to open file for reading");
      return false;
    }
    Serial.println(file.readString());
    file.close();
    return true;
  }
  Serial.println("file is not exists");
  return false;
}

bool printContentFileFromSD(String file_name) {
  if (!SD.exists(file_name))
    return false;

  File file = SD.open(file_name, FILE_READ);
  if (!file) {
    Serial.print("error opening ");
    Serial.println(file_name);
    while (1);
  }
  Serial.println(file.readString());
  // close the file:
  file.close();
  return false;
}

void printNUID() {
  Serial.print(mfrc522.uid.uidByte[0], HEX);
  Serial.print(" ");
  Serial.print(mfrc522.uid.uidByte[1], HEX);
  Serial.print(" ");
  Serial.print(mfrc522.uid.uidByte[2], HEX);
  Serial.print(" ");
  Serial.println(mfrc522.uid.uidByte[3], HEX); 
}
