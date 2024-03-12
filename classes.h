#ifndef PROCEDURES_H
#define PROCEDURES_H
#define DEBUG 0
// #define DEBUG 1
#if DEBUG == 1
#define Debug(x) Serial.print(x)
#define Debugln(x) Serial.println(x)
#define Debugf(x) Serial.printf(x)
#else
#define Debug(x)
#define Debugln(x)
#define Debugf(x)
#endif

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>
#include <time.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <credentials.h>
// #define MYSSID .....
// #define MYPASSWORD ......

class Record {
#define FILE_WRITE_BUF_SIZE 1024
// #define FILE_WRITE_BUF_SIZE 64
#define CS 22  //SD card chip select port
// #define CS 15  //SD card chip select port
#define LED_BUILTIN 2
#define LED LED_BUILTIN
#define BUTTON 0  //On board boot button
  File configFile;
  File audioFile;
  JsonDocument doc;
  unsigned long recordingStartTime = 0;  // Stores recording start time
  unsigned long recordingDuration;
  struct tm tmstruct;
  const char* weekDays[7] PROGMEM = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  
  struct Config {
    char wifiSsid[15];
    char wifiPassword[15];
    char host[32];
    char path[32];
    int port;
    char format[5];
    int startHour;
    int startMinute;
    int recordLength;
  };

  Config configuration;
  long timezone = 1;
  byte daysavetime = 0;
  WiFiClient client;
  bool recordingStarted = false;
  bool loadConfiguration(Config& config, const char* filename);
  bool mountSDCard();
  void unmountSDCard();
  void listDir(fs::FS& fs, const char* dirname, uint8_t levels);
  bool openAudioFile();
  void showFiles();

public:
  bool loadConfiguration(const char* filename = "/config.txt");
  void printConfiguration();
  bool connectWifi();
  bool awaitingRecordingTime();
  bool connectToRadio();
  bool recordingStart();
  bool recordingProgress();
  bool togglePort(uint8_t port, unsigned int duration = 200, unsigned int times = 1);
};

#endif
