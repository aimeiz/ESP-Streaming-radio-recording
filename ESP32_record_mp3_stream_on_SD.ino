/* Program for recording audio stream from internet radio web pages on SD card, at set time and set length.
Press boot button for initialize imidiate recording start
SD Card format FAT32
SD Size - ~1MB / each recording minute. Reasonable size 1GB.

Program reads configuration file config.txt stored on SD card with wifi and recording parameters
config.txt content example - brackets{} are mandatory:
{
 "wifiSsid": "Your wifi ssid",
 "wifiPassword": "Your wifi password",
"host": "stream13.radioagora.pl",
"path": "/tuba38-1.mp3",
"port": 80,
"format": ".mp3",
"startHour": 12,
"startMinute": 0,
"recordLength": 10
}
If file is missing, program takes default hardcoded configuration, and still is able to record if proper SD card is connected.
Default Wifi ssid and password should be set either by
#include <credentials.h> with defined  MYSSID MYPASSWORD macros or just coded in classes.h file as
#define MYSSID .....
#define MYPASSWORD ......
to allow recording on empty SD - without config.txt file.
Hardware Connections:
SPI SD card:
SD    ESP32
GND   GND
CLK   D18
MISO  D19
CS    D22
MOSI  D23
+3.3V +3.3V
LED - D2 - If not present on ESP board connect LED - anode with serial resistor 220 ohms, cathode - GND
BUTTON 0 and GND if not accessible on ESP board.

Program has been developed and tested using ESP32 Dev Module.
*/
// Config configuration examples:
/*++++++ Radio Maryja +++++++++
configuration.wifiSsid = MYSSID;
configuration.wifiPassword = MYPASSWORD;
configuration.host = "51.68.135.155";
configuration.path = "/stream";
configuration.port = 80
configuration.format = ".mp4"
configuration.startHour = 12;
configuration.startMinute = 15;
configuration.recordLength = 30;
++++++++++++++++++++++++++++*/

/******Radio Pogoda ********
configuration.wifiSsid = MYSSID;
configuration.wifiPassword = MYPASSWORD;
configuration.host = "stream13.radioagora.pl";
configuration.path = "/tuba38-1.mp3";
configuration.port = 80
configuration.format = ".mp3"
configuration.startHour = 13;
configuration.startMinute = 0;
configuration.recordLength = 30;
******************************/

#include "classes.h"  //Most of magic is in this file
Record recordStream;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\nProgram started"));
  digitalWrite(LED, LOW);
  if (!recordStream.loadConfiguration("/config.txt"))
    return;
  recordStream.printConfiguration();

  //For testing purposes possible to override below selected configuration items loaded from SD
  /*++++++ Radio Maryja +++++++++*/
  // recordStream.configuration.wifiSsid = MYSSID;
  // recordStream.configuration.wifiPassword = MYPASSWORD;
  // recordStream.configuration.host = "stream13.radioagora.pl";
  // recordStream.configuration.path = "/tuba38-1.mp3";
  // recordStream.configuration.port = 80;
  // recordStream.configuration.format = ".mp3";
  // recordStream.configuration.startHour = 20;
  // recordStream.configuration.startMinute = 20;
  // recordStream.configuration.recordLength = 3;

  if (!recordStream.connectWifi())
    return;
  if (!recordStream.awaitingRecordingTime())
    Serial.println(F("\nIt's time for recording"));
  if (!recordStream.connectToRadio())
    return;
  if (!recordStream.recordingStart())
    return;
}

void loop() {
  if (!recordStream.recordingProgress()) {
    recordStream.togglePort(LED, 100, 20);  //Waits 2 sek blinking fast instead of delay
    ESP.restart();                          //Restarts ESP32 for next recording
  }
}
