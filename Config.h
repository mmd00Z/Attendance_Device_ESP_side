#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#define DEFAULT_CONFIG_FILE_NAME "/config.txt"

class Config {  
  public:
  struct Time {
    int hour;
    int minute;
    int seconds;
  };

  String access_password; // Password for login to application and check the access when transfer the data
  String email_address;   // Someone to whom you should email the attendance list

  char* ssid;     // Network name
  char* password; // Network preshared key

  Time start_time; // Start time of attendance
  Time end_time;   // End time of attendance
  bool isManualStarted;

  unsigned int storage_time; // By month

  bool send_email;
  bool ability_of_blind;
  bool call_to_parent;
  bool massage_to_parent;

  Config(int debuger);

  bool saveConfigurations(String path);

  bool loadConfigurations(String path);

  void printAllConfigs();
};

#endif