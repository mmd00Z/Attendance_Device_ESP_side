#include "Config.h"

Config::Config(int debuger) {
    ssid = "ghazanfar";
    password = "12345678";
    access_password = "1234";
    email_address = "mmd.gh313@gmail.com";
    start_time.hour = 7;
    start_time.minute = 30;
    start_time.seconds = 0;
    end_time.hour = 8;
    end_time.minute = 0; 
    end_time.seconds = 0;
    storage_time = 6;
    send_email = false;  
    ability_of_blind = false; 
    call_to_parent = false;
    massage_to_parent = false;
  }

  bool Config::saveConfigurations(String path) {
    // write json object to file
    File file = SPIFFS.open(path, "w");
    if(!file){ 
      Serial.println("Failed to open file for reading");
      return false;
    }
    Serial.println("file is ready! saving...");

    DynamicJsonDocument doc(1024);

    doc["SSID"]            = ssid;
    doc["Pasword"]         = password;
    doc["access_password"] = access_password; 
    doc["email_address"]   = email_address; 
    doc["start_hour"]      = start_time.hour;
    doc["start_min"]       = start_time.minute;
    doc["start_sec"]       = start_time.seconds;
    doc["end_hour"]        = end_time.hour;
    doc["end_min"]         = end_time.minute;
    doc["end_sec"]         = end_time.seconds;
    doc["storage_time"]    = storage_time;
    doc["send_email"]      = send_email;
    doc["ability_blind"]   = ability_of_blind;
    doc["call_parent"]     = call_to_parent;
    doc["massage_parent"]  = massage_to_parent;

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) 
      Serial.println("Failed to write to file");
    
    file.close();
    Serial.println("fSave compleated");
    return true;
  }

  bool Config::loadConfigurations(String path) {
    if(!SPIFFS.exists(path)){
      Serial.println("file is not exists! loding...");
      return false;
    }
    Serial.println("file is exists.");

    File file = SPIFFS.open(path, "r");
    if(!file){ 
      Serial.println("Failed to open file for reading");
      return false;
    }

    Serial.println("file is open.");

    DynamicJsonDocument doc(1024);

    DeserializationError error = deserializeJson(doc, file);
    if (error)
      Serial.println("Failed to read file, using default configuration");

    Serial.println("deserializeJson compleated");

    // extract the json object to config varibels
    strcpy(ssid, doc["SSID"]);
    strcpy(password, doc["Pasword"]);
    access_password    = doc["access_password"].as<String>(); 
    email_address      = doc["email_address"].as<String>(); 
    start_time.hour    = doc["start_hour"];
    start_time.minute  = doc["start_min"];
    start_time.seconds = doc["start_sec"];
    end_time.hour      = doc["end_hour"];
    end_time.minute    = doc["end_min"];
    end_time.seconds   = doc["end_sec"];
    storage_time       = doc["storage_time"];
    send_email         = doc["send_email"];
    ability_of_blind   = doc["ability_blind"];
    call_to_parent     = doc["call_parent"];
    massage_to_parent  = doc["massage_parent"];
    
    file.close();
    Serial.println("load compleated!");
    return true;
  }

  void Config::printAllConfigs() {
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Pasword: ");
    Serial.println(password);
    Serial.print("access password: ");
    Serial.println(access_password);
    Serial.print("email address: ");
    Serial.println(email_address);
    Serial.print("Start Time: ");
    Serial.print(start_time.hour); Serial.print(":"); Serial.print(start_time.minute); Serial.print(":"); Serial.println(start_time.seconds);
    Serial.print("End Time: ");
    Serial.print(end_time.hour); Serial.print(":"); Serial.print(end_time.minute); Serial.print(":"); Serial.println(end_time.seconds);
    Serial.println("storage_time: ");
    Serial.println(storage_time);
    Serial.println("send_email: ");
    Serial.println(send_email);
    Serial.println("ability_blind: ");
    Serial.println(ability_of_blind);
    Serial.println("call_parent: ");
    Serial.println(call_to_parent);
    Serial.println("massage_parent: ");
    Serial.println(massage_to_parent);
  }