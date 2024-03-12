#include "classes.h"

bool Record::togglePort(uint8_t port, unsigned int duration, unsigned int times) {
  bool state;
  pinMode(LED, OUTPUT);
  for (unsigned int i = 0; i < times; i++) {
    state = !digitalRead(port);
    digitalWrite(port, state);
    delay(duration);
  }
  return state;
}

bool Record::loadConfiguration(Config& config, const char* filename) {
  if (!mountSDCard())
    // return false;
    Debug(F("Trying to open file "));
  Debug(filename);
  Debugln();
  if (!(configFile = SD.open(filename))) {  // Open file for reading
    Debugln(F("Failed to open file"));
    unmountSDCard();
    // return false;
  }
  else
  {
  Serial.print(F("Configuration file "));
  Serial.print(filename);
  Serial.println(F(" open"));
  }
  // Allocate a temporary JsonDocument
  // const size_t bufferSize = JSON_OBJECT_SIZE(9) + 196;
  // DynamicJsonDocument doc(bufferSize);
  // JsonDocument doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println(F("Failed to read file, using default configuration"));
    configFile.close();
    unmountSDCard();
    // return false;
  }
  Debugln(F("Deserlization completed"));
  // Copy values from the JsonDocument to the Config
  strlcpy(config.wifiSsid,                   // <- destination
          doc["wifiSsid"] | MYSSID,          // <- source
          sizeof(config.wifiSsid));          // <- destination's capacity
  strlcpy(config.wifiPassword,               // <- destination
          doc["wifiPassword"] | MYPASSWORD,  // <- source
          sizeof(config.wifiPassword));      // <- destination's capacity
  Debugln(F("Wifi ssid, password loaded"));
  strlcpy(config.host,                             // <- destination
          doc["host"] | "stream13.radioagora.pl",  // <- source
          sizeof(config.host));                    // <- destination's capacity
  strlcpy(config.path,                             // <- destination
          doc["path"] | "/tuba38-1.mp3",           // <- source
          sizeof(config.path));                    // <- destination's capacity
  config.port = doc["port"] | 80;
  strlcpy(config.format,           // <- destination
          doc["format"] | ".mp3",  // <- source
          sizeof(config.format));  // <- destination's capacity
  Debugln(F("Stream data loaded"));
  config.startHour = doc["startHour"] | 12;
  config.startMinute = doc["startMinute"] | 0;
  config.recordLength = doc["recordLength"] | 3;
  Debugln(F("Recording time loaded"));

  configFile.close();  // Close the file (Curiously, File's destructor doesn't close the file)
  unmountSDCard();
  Debugln(F("config structure filled"));
  togglePort(LED, 200, 5);  //blik 5 times if loading configuration success
  return true;
}

bool Record::loadConfiguration(const char* filename) {
  if (!loadConfiguration(configuration, filename))
    return false;
  return true;
}

bool Record::mountSDCard() {
  Debugln(F("Mounting SD card..."));
  if (!SD.begin(CS)) {
    Debugln(F("SD card Mount Failed"));
    return false;
  }
  Debugln(F("SD card mounted."));
  return true;
}

void Record::unmountSDCard() {
  SD.end();
  Debugln(F("SD card Unmounted"));
}

void Record::printConfiguration() {
  // Serial.printf("wifiSsid: %s wifiPassword %s ", configuration.wifiSsid, configuration.wifiPassword);
  Serial.printf("\nwifiSsid: %s wifiPassword %s ", configuration.wifiSsid, "********");
  Serial.printf("host: %s path: %s port: %d startHour: %02d startMinute: %02d, recordLenght: %d\n", configuration.host, configuration.path, configuration.port, configuration.startHour, configuration.startMinute, configuration.recordLength);
}

void Record::listDir(fs::FS& fs, const char* dirname, uint8_t levels) {
  if (!mountSDCard()) return;
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println(F("Failed to open directory"));
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(F("Not a directory"));
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print(F("  DIR : "));
      Serial.print(file.name());
      time_t t = file.getLastWrite();
      struct tm* tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print(F("  FILE: "));
      Serial.print(file.name());
      Serial.print(F("  SIZE: "));
      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm* tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
  unmountSDCard();
}

bool Record::connectToRadio() {
  if (!client.connect(configuration.host, configuration.port)) {
    Serial.println(F("Connection to radio failed"));
    return false;
  }
  Serial.print(F("Connected to radio. "));
  Serial.print(configuration.host);
  Serial.print(F(" on port "));
  Serial.print(configuration.port);
  Serial.print(F(" Path "));
  Serial.println(configuration.path);
  return true;
}

bool Record::openAudioFile() {
  if (!mountSDCard())
    return false;
  char fileName[32];
  sprintf(fileName, "/%d%02d%02d%s%02d%02d%02d%s", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, weekDays[tmstruct.tm_wday], tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec, configuration.format);
  Serial.print(F("Opening record file "));
  Serial.println(fileName);
  audioFile = SD.open(fileName, FILE_WRITE);
  if (!audioFile) {
    Serial.println(F("Failed to open audio file"));
    unmountSDCard();
    return false;
  }
  return true;
}

void Record::showFiles() {
  listDir(SD, "/", 0);
}


bool Record::recordingStart() {
  recordingStartTime = millis();  // set recording start time
  //Read stream from internet radio and store it on SD card
  if (!openAudioFile())
    return false;
  client.print(String("GET ") + configuration.path + " HTTP/1.1\r\n" + "Host: " + configuration.host + "\r\n" + "Connection: close\r\n\r\n");
  Serial.print(F("Recording started for "));
  Serial.print(configuration.recordLength);
  Serial.println(F(" minutes "));
  recordingDuration = configuration.recordLength * 60000;
  recordingStarted = true;
  return true;
}

bool Record::recordingProgress() {
  static unsigned long previousTime1;
  static unsigned long previousTime2;
  const unsigned long interval1 = 200;
  const unsigned long interval2 = 2000;
  unsigned long currentTime = millis();
  static unsigned int bytesStored;
  if (!recordingStarted) {
    Serial.println(F("Failed to start recording"));
    return false;
  }
  if (currentTime - recordingStartTime >= recordingDuration) {
    Serial.println(F("Recording stopped due to duration limit reached."));
    audioFile.close();
    unmountSDCard();
    digitalWrite(LED_BUILTIN, LOW);
    showFiles();
    getLocalTime(&tmstruct, 5000);
    Serial.printf("\nNow is : %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
    bytesStored = 0;
    return false;  //Returns false if recording finished
  }
  if (currentTime - previousTime1 >= interval1) {
    togglePort(LED);  //toggle on board LED to indicate recording in progress
    previousTime1 = currentTime;
  }
  if (currentTime - previousTime2 >= interval2) {
    Serial.print(F("Kbytes stored "));
    Serial.print(bytesStored / 1024);
    Serial.print(F(" Seconds to finish "));
    Serial.println(((recordingDuration + recordingStartTime - currentTime) / 1000));
    previousTime2 = currentTime;
  }
  while (client.available()) {
    uint8_t mp3Data[FILE_WRITE_BUF_SIZE];
    int bytesRead = client.read(mp3Data, FILE_WRITE_BUF_SIZE);
    if (bytesRead > 0) {
      audioFile.write(mp3Data, bytesRead);
      bytesStored += bytesRead;
    }
  }
  delay(10);    // Wait a moment before check if something is ready to store
  return true;  //Returns false if recordingis in progress
}

bool Record::awaitingRecordingTime() {
  static unsigned long previousTime;
  unsigned long currentTime;
  const unsigned long interval = 5000;
  bool buttonPressed;
  pinMode(BUTTON, INPUT_PULLUP);
  Debugln(F("Contacting Time Server"));
  do {  //Awaiting for recording start time
    buttonPressed = !digitalRead(BUTTON);
    currentTime = millis();
    if ((currentTime - previousTime) >= interval) {
      configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
      tmstruct.tm_year = 0;
      if (getLocalTime(&tmstruct, 5000)) {
        togglePort(LED);
        Serial.printf("\nNow is : %s %d-%02d-%02d %02d:%02d:%02d ", weekDays[tmstruct.tm_wday], (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
        Serial.printf("Record will start at %02d:%02d:00", configuration.startHour, configuration.startMinute);
      }
      previousTime = currentTime;
    }
  } while (((configuration.startHour != tmstruct.tm_hour) || (configuration.startMinute != tmstruct.tm_min)) && !buttonPressed);
  buttonPressed = false;
  return false;
}

bool Record::connectWifi() {
  Serial.print(F("Connecting to WiFi "));
  Serial.println(configuration.wifiSsid);
  WiFi.begin(configuration.wifiSsid, configuration.wifiPassword);
  for (uint8_t i = 0; (i < 50) && (WiFi.status() != WL_CONNECTED); i++) {
    Serial.print(".");
    togglePort(LED, 1000);
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("Unable to connect to WiFi "));
    Serial.println(configuration.wifiSsid);
    togglePort(LED, 100, 20);
    return false;
  }
  togglePort(LED, 1000, 5);
  Serial.print(F("Connected to WiFi."));
  Serial.print(configuration.wifiSsid);
  Serial.print(F(" Ip address: "));
  Serial.println(WiFi.localIP());
  return true;
}
