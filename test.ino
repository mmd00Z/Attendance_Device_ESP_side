#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include "db_methods.h"
#include "network_data.h"
#include "Config.h"

#define BLOCKA_ADDRES 2
#define TRAILER_BLOCK 3
#define SS_PIN 2
#define RST_PIN 4

const long utcOffsetInSeconds = 3600 * 3.5;

IPAddress local_IP(192, 168, 43, 100); // Static IP
// IPAddress local_IP(192, 168, 146, 150); // Static IP
IPAddress gateway (192, 168, 43, 1);   // GatewayIP
// IPAddress gateway (192, 168, 146, 131);   // GatewayIP
IPAddress subnet  (255, 255, 255, 0);  // Subnet MASK

ESP8266WebServer server(80);

LinkedList <Student*> students = LinkedList <Student*>();
LinkedList <Student*> present_list = LinkedList <Student*>();

MFRC522 mfrc522(SS_PIN, RST_PIN);
WiFiUDP ntpUDP;
Config conf(5);
NTPClient local_time(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup() {
  // conf.init();
  Serial.begin(115200);
  Serial.println("SPI begining...");

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  conf.loadConfigurations(DEFAULT_CONFIG_FILE_NAME);
  Serial.println(printContentFile(DEFAULT_CONFIG_FILE_NAME));
  Serial.println("some loaded data: ");
  Serial.println(conf.access_password);
  Serial.println(conf.email_address);
  Serial.println(conf.ssid);
  Serial.println(conf.password);
  Serial.println(conf.call_to_parent);
  Serial.println(conf.end_time.hour);

  SPI.begin();
  mfrc522.PCD_Init();
  delay(4); // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  WiFi.disconnect();
  if (!WiFi.config(local_IP, gateway, subnet)) 
    Serial.println("Failed to conf.re");
    
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
  if(students.size() > 0) students.remove(0);
  show_all_student();

  local_time.begin();
}

void loop() {
  local_time.update();
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
      }
    }
  }
}

bool connectToNetwork() {
}

int8_t isInPresentList(String ncode) { // get national code and return true if the onwner of the ncode be in the present list
  for(uint16_t i=0; i<present_list.size(); i++) 
    if(present_list.get(i)->getNationalCode() == ncode)
      return i;
  return -1;
}

int8_t isInStudentsList(String ncode) { // get national code and return true if the onwner of the ncode be in the students list
  for(uint16_t i=0; i<students.size(); i++) 
    if(students.get(i)->getNationalCode() == ncode)
      return i;
  return -1;
}

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
  show_all_student();
}

void handlePresent() {
  String presents = pack_string_property(&present_list, StudentProperty::NATIONAL_CODE, '*');
  Serial.println(presents);
  server.send(200, "text/plain", presents);
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
  ((local_time.getHours() > conf.start_time.hour || (local_time.getHours() == conf.start_time.hour && local_time.getMinutes() >= conf.start_time.minute)) 
  && (local_time.getHours() < conf.end_time.hour || (local_time.getHours() == conf.end_time.hour && local_time.getMinutes() <= conf.end_time.minute)));
  return true;
}

void show_all_student() {
  // Serial.println(F("_______________"));
  if(students.size() <= 0) 
    Serial.println("No students added!");
  else {
    for(int i=0; i<students.size(); i++) {
      Serial.print(i+1);
      Serial.println(". "+students.get(i)->getFirstName()+"\t"+students.get(i)->getLastName()+"\t"+students.get(i)->getNumberPhone()+"\t"+students.get(i)->getNationalCode());
    }
  }
  // Serial.println(F("_______________"));
}

bool print_file_data(String path) {
  if(SPIFFS.exists(path)){
    File name_file = SPIFFS.open(path, "r");
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

bool printContentFile(String path) {
  if(SPIFFS.exists(path)){
    // file is exist
    File file = SPIFFS.open(path, "r");
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

void printNUID() {
  Serial.print(mfrc522.uid.uidByte[0], HEX);
  Serial.print(" ");
  Serial.print(mfrc522.uid.uidByte[1], HEX);
  Serial.print(" ");
  Serial.print(mfrc522.uid.uidByte[2], HEX);
  Serial.print(" ");
  Serial.println(mfrc522.uid.uidByte[3], HEX); 
}
