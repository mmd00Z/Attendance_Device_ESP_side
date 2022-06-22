#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <SNTPtime.h>
#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <UnixTime.h>
#include "db_methods.h"
#include "network_data.h"
#include "Config.h"


#define BLOCKA_ADDRES 2
#define TRAILER_BLOCK 3
#define RFID_SS_PIN 2
#define RST_PIN 4
#define SD_CARD_SS_PIN 16
#define SELECTED_NTP_SERVER "ntp.day.ir"
#define IRAN_UTC_OFFSET 4.5
#define DIRECTOR_PATH_NAME "/db/"
/*
 "ntp.day.ir"
 "pool.ntp.org"
 "pool.ntp.org"
 "pool.ntp.org"
 "pool.ntp.org"
*/

const long utcOffsetInSeconds = 3600 * IRAN_UTC_OFFSET;
const int UTC_OFFSET =  4.5;

//Week Days
const String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
const String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};


LinkedList <Student*> students = LinkedList <Student*>();
LinkedList <Student*> present_list = LinkedList <Student*>();
String present_str = "";

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
// NTPClient timeClient("europe.pool.ntp.org", 3600, 60000);
// SNTPtime ntpTime("ntp.day.ir");
// SNTPtime ntpTime(SELECTED_NTP_SERVER);
// strDateTime dateTime;

UnixTime date_time(0);

void setup() {
  // conf.init();
  Serial.begin(115200);
  Serial.println("SPI begining...");

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

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet, DNS1, DNS2)) 
    Serial.println("WiFi config failed");
    
  Serial.println("Connecting to AccessPoint");
  WiFi.begin(conf.ssid, conf.password);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("WiFi Connected!!");
  Serial.println("\nIP: "+WiFi.localIP().toString());
  Serial.println("GatewayIP: "+WiFi.gatewayIP().toString());
  Serial.println("Hostname: "+WiFi.hostname());
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
      }
    }
  }
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

String getFormatedTime() {
  return String(date_time.hour)+":"+String(date_time.minute)+":"+String(date_time.second);
}

bool connectToNetwork() {}

void addStudent() {
  String Fname = server.arg("FName");
  String Lname = server.arg("LName");
  String number_phone = server.arg("Number");
  String national_code = server.arg("Code");
  waitForCard();
  writeNationalCode(national_code);
  students.add(new Student(Fname, Lname, number_phone, national_code));
  save(&students);
  server.send(200, "text/plain", "Student added!");
  Student::showAllStudent(&students);
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
  Serial.println("Reseted Factory...");
  SPIFFS.format();
  server.send(200, "text/plain", "Reseted Factory");
  ESP.restart();
}

void manualAttendance() {
  conf.isManualStarted = (server.arg("is_manual_started") == "true")? true:false;
  if(conf.isManualStarted)
    server.send(200, "text/plain", "Attendance began");
  else
    server.send(200, "text/plain", "Attendance is over");
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
  while (!mfrc522.PICC_IsNewCardPresent());
  while (!mfrc522.PICC_ReadCardSerial());
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
