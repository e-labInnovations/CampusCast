#include <FS.h>          // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <SPIFFS.h>
#include <WebSocketsClient.h>

#include "WiFi.h"
#include "Audio.h"

#define I2S_DOUT 12//25
#define I2S_BCLK 13//27
#define I2S_LRC 15//26

// const char* ssid = "e-labinnovations";
// const char* password = "PASSWORD";

Audio audio;
WiFiManager wm;
WebSocketsClient webSocket;



char classroom_code[10];

bool shouldSaveConfig = true;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setupSpiffs() {
  // Uncomment if we need to format filesystem
  SPIFFS.format();

  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json");
      if (configFile) {
        Serial.println("Opened configuration file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error) {
          Serial.println("Parsing JSON");

          strcpy(classroom_code, json["classroom_code"]);
        } else {
          // Error loading JSON data
          Serial.println("Failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      webSocket.sendTXT("Connected");
      break;
    case WStype_TEXT:
      USE_SERIAL.printf("[WSc] get text: %s\n", payload);

      // send message to server
      // webSocket.sendTXT("message here");
      break;
    case WStype_BIN:
      USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
      
      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
    case WStype_ERROR:      
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }

}
void setup() {
  
//  wm.resetSettings();

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  setupSpiffs();
  bool res;

  wm.setSaveConfigCallback(saveConfigCallback);
  WiFiManagerParameter custom_classroom_code("classroom_code", "Classroom No", classroom_code, 10);

  wm.addParameter(&custom_classroom_code);

  res = wm.autoConnect("CampusCast", "password");  // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
  strcpy(classroom_code, custom_classroom_code.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    StaticJsonDocument<512> json;
    json["classroom_code"] = classroom_code;

    // json["ip"]          = WiFi.localIP().toString();
    // json["gateway"]     = WiFi.gatewayIP().toString();
    // json["subnet"]      = WiFi.subnetMask().toString();

    File configFile = SPIFFS.open("/config.json", FILE_WRITE);
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    // Serialize JSON data to write to file
    serializeJsonPretty(json, Serial);
    if (serializeJson(json, configFile) == 0) {
      Serial.println(F("Failed to write to file"));
    }
    configFile.close();
    //end save
    shouldSaveConfig = false;
  }

  webSocket.begin("192.168.1.47", 1880, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  /*audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21);
    audio.connecttohost("http://vis.media-ice.musicradio.com/CapitalMP3");
    // audio.connecttohost("https://firebasestorage.googleapis.com/v0/b/campuscast-elabins.appspot.com/o/recording-test01.m4a?alt=media&token=2e25980f-1d7c-4675-9a52-67a0c65170fb");
  */
}

void loop() {
  //audio.loop();
  webSocket.loop();
}

// optional
void audio_info(const char *info) {
  Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info) { //id3 metadata
  Serial.print("id3data     "); Serial.println(info);
}
void audio_eof_mp3(const char *info) { //end of file
  Serial.print("eof_mp3     "); Serial.println(info);
}
void audio_showstation(const char *info) {
  Serial.print("station     "); Serial.println(info);
}
void audio_showstreamtitle(const char *info) {
  Serial.print("streamtitle "); Serial.println(info);
}
void audio_bitrate(const char *info) {
  Serial.print("bitrate     "); Serial.println(info);
}
void audio_commercial(const char *info) { //duration in sec
  Serial.print("commercial  "); Serial.println(info);
}
void audio_icyurl(const char *info) { //homepage
  Serial.print("icyurl      "); Serial.println(info);
}
void audio_lasthost(const char *info) { //stream URL played
  Serial.print("lasthost    "); Serial.println(info);
}
void audio_eof_speech(const char *info) {
  Serial.print("eof_speech  "); Serial.println(info);
}
