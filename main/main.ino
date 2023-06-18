#include <FS.h>           // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <SPIFFS.h>
#include <WebSocketsClient.h>  //version 2.3.6
#include "WiFi.h"
#include "Audio.h"

#define SSID_PREFIX "CampusCast_"
#define PASSWORD "password"
#define JSON_CONFIG_FILE "/config.json"

#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC 25
#define BTN_CONFIG_RESET 0
#define BTN_ACK 34
#define RED_LED 19
#define GREEN_LED 18

Audio audio;
WiFiManager wm;
WebSocketsClient webSocket;

bool shouldSaveConfig = true;
char classroom_code_val[10] = "000";
char ws_server_val[21] = "192.168.x.x";
int ws_server_port_val = 1880;
char ws_server_path_val[41] = "/";

bool playAudio = false;
String audioURL = "";
String anncmntId = "";
bool prevAckStatus = true;

enum IndicatorStatus {
  START_MODE,
  CONFIG_MODE,
  WIFI_NOT_CONN,
  WIFI_CONN_SRV_NOT_CONN,
  WIFI_SRV_OK
};

struct Indicator {
  int redLedOldStatus;
  int greenLedOldStatus;
  IndicatorStatus currentStatus;
  unsigned long previousMillis;
};
Indicator indicator;  // Declare an instance of the Indicator structure


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);

  pinMode(BTN_CONFIG_RESET, INPUT);
  pinMode(BTN_ACK, INPUT);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // Initialize the Indicator structure
  indicator.redLedOldStatus = LOW;
  indicator.greenLedOldStatus = LOW;
  indicator.currentStatus = START_MODE;
  indicator.previousMillis = 0;
  setIndicatorStatus(START_MODE);

  bool forceConfig = false;  // Change to true when testing to force configuration every time we run

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup) {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  setupWiFiManager();

  // server address, port and URL
  webSocket.begin(ws_server_val, ws_server_port_val, ws_server_path_val);
  // event handler
  webSocket.onEvent(webSocketEvent);
  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  // webSocket.enableHeartbeat(15000, 3000, 2);

  Serial.print("WS server: ");
  Serial.println(ws_server_val);
  Serial.print("WS server port: ");
  Serial.println(ws_server_port_val);
  Serial.print("WS server path: ");
  Serial.println(ws_server_path_val);
  Serial.print("Classroom Code: ");
  Serial.println(classroom_code_val);

  // audio.connecttospeech("WiFi Connected", "en");
}

void loop() {
  webSocket.loop();
  audio.loop();

  if (playAudio) {
    Serial.print("audioUrl: ");
    Serial.println(audioURL);
    Serial.print("Id: ");
    Serial.println(anncmntId);
    webSocket.disconnect();
    audio.connecttohost(audioURL.c_str());
    playAudio = false;
  }

  if (!digitalRead(BTN_CONFIG_RESET)) {
    Serial.println("BTN_CONFIG_RESET pressed, resetting WiFi credentials");
    wm.resetSettings();  // reset settings - wipe stored credentials for testing
    ESP.restart();       // restart eps32
  }

  bool presAckStatus = digitalRead(BTN_ACK);
  if (prevAckStatus && !presAckStatus) {
    Serial.println("BTN_ACK pressed");

    if (anncmntId != "") {
      webSocket.sendTXT("{ \"command\":\"ack\", \"classroom_code\": \"" + String(classroom_code_val) + "\", \"msg_id\":\"" + anncmntId + "\" }");
      anncmntId = "";
    }
    delay(10);
  }
  prevAckStatus = presAckStatus;
}

void handleIncomingData(uint8_t *json) {
  DynamicJsonDocument recvData(500);
  DeserializationError error = deserializeJson(recvData, json);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  String command = recvData["command"];
  Serial.println(command);

  if (command == "anncmnt") {
    String recipient = recvData["recipient"];
    if (recipient == String(classroom_code_val)) {
      const char *_audioURL = recvData["audioUrl"];
      const char *_anncmntId = recvData["id"];
      audioURL = String(_audioURL);
      anncmntId = String(_anncmntId);
      playAudio = true;
    }
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      webSocket.sendTXT("{ \"command\":\"connected\", \"classroom_code\": \"" + String(classroom_code_val) + "\" }");
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);

      handleIncomingData(payload);
      break;
  }
}

void saveConfigFile() {
  Serial.println(F("Saving configuration..."));

  // Create a JSON document
  StaticJsonDocument<512> json;
  json["ws_server"] = ws_server_val;
  json["ws_server_port"] = ws_server_port_val;
  json["ws_server_path"] = ws_server_path_val;
  json["classroom_code"] = classroom_code_val;

  // Open config file
  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  // Serialize JSON data to write to file
  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0) {
    Serial.println(F("Failed to write to file"));
  }
  // Close file
  configFile.close();
}

// Load existing configuration file
bool loadConfigFile() {
  // Uncomment if we need to format filesystem
  // SPIFFS.format();

  // Read configuration from FS json
  Serial.println("Mounting File System...");

  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin(true)) {
    Serial.println("mounted file system");
    if (SPIFFS.exists(JSON_CONFIG_FILE)) {
      // The file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile) {
        Serial.println("Opened configuration file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error) {
          Serial.println("Parsing JSON");

          strcpy(ws_server_val, json["ws_server"]);
          ws_server_port_val = json["ws_server_port"].as<int>();
          strcpy(ws_server_path_val, json["ws_server_path"]);
          strcpy(classroom_code_val, json["classroom_code"]);
          return true;
        } else {
          // Error loading JSON data
          Serial.println("Failed to load json config");
        }
      }
    }
  } else {
    // Error mounting file system
    Serial.println("Failed to mount FS");
  }
  return false;
}

// Callback notifying us of the need to save configuration
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// Called when config mode launched
void configModeCallback(WiFiManager *myWiFiManager) {
  setIndicatorStatus(CONFIG_MODE);
  Serial.println("Entered Configuration Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}


void setupWiFiManager() {
  // wm.resetSettings(); // reset settings - wipe stored credentials for testing
  wm.setSaveConfigCallback(saveConfigCallback);  // Set config save notify callback
  wm.setAPCallback(configModeCallback);          // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setDebugOutput(false);                      //Disable debugging
  wm.setDarkMode(true);                          // dark theme

  WiFiManagerParameter ws_server("ws_server", "Websocket Server Address", ws_server_val, 20);
  wm.addParameter(&ws_server);

  char ws_server_port_val_a[16];
  WiFiManagerParameter ws_server_port("ws_server_port", "Websocket Server Port", itoa(ws_server_port_val, ws_server_port_val_a, 10), 5);
  wm.addParameter(&ws_server_port);

  WiFiManagerParameter ws_server_path("ws_server_path", "Websocket Server Path", ws_server_path_val, 40);
  wm.addParameter(&ws_server_path);

  WiFiManagerParameter classroom_code("classroom_code", "Classroom No", classroom_code_val, 10);
  wm.addParameter(&classroom_code);

  bool res;
  String AP_NAME = SSID_PREFIX + String(classroom_code_val);
  res = wm.autoConnect(AP_NAME.c_str(), PASSWORD);  // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  }

  setIndicatorStatus(WIFI_CONN_SRV_NOT_CONN);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  strncpy(ws_server_val, ws_server.getValue(), sizeof(ws_server_val));                 // Copy the string value
  ws_server_port_val = atoi(ws_server_port.getValue());                                //Convert the number value
  strncpy(ws_server_path_val, ws_server_path.getValue(), sizeof(ws_server_path_val));  // Copy the string value
  strcpy(classroom_code_val, classroom_code.getValue());

  Serial.print("WS server: ");
  Serial.println(ws_server.getValue());
  Serial.print("WS server port: ");
  Serial.println(ws_server_port.getValue());
  Serial.print("WS server path: ");
  Serial.println(ws_server_path.getValue());
  Serial.print("Classroom Code: ");
  Serial.println(classroom_code.getValue());
  // Save the custom parameters to FS
  if (shouldSaveConfig) {
    saveConfigFile();
  }
}

void setIndicatorStatus(IndicatorStatus status) {
  switch (status) {
    case START_MODE:
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      break;
    case CONFIG_MODE:
      // Set RED LED ON
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
      break;
    case WIFI_NOT_CONN:
      // Set RED LED BLINKING
      // ...
      break;
    case WIFI_CONN_SRV_NOT_CONN:
      // Set RED & GREEN LED ALTERNATE BLINKING
      // ...
      break;
    case WIFI_SRV_OK:
      // Set GREEN LED ON
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);
      break;
  }
}

// optional
void audio_info(const char *info) {
  Serial.print("info        ");
  Serial.println(info);
}
void audio_id3data(const char *info) {  //id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);
}
void audio_eof_mp3(const char *info) {  //end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
}
void audio_showstation(const char *info) {
  Serial.print("station     ");
  Serial.println(info);
}
void audio_showstreamtitle(const char *info) {
  Serial.print("streamtitle ");
  Serial.println(info);
}
void audio_bitrate(const char *info) {
  Serial.print("bitrate     ");
  Serial.println(info);
}
void audio_commercial(const char *info) {  //duration in sec
  Serial.print("commercial  ");
  Serial.println(info);
}
void audio_icyurl(const char *info) {  //homepage
  Serial.print("icyurl      ");
  Serial.println(info);
}
void audio_lasthost(const char *info) {  //stream URL played
  Serial.print("lasthost    ");
  Serial.println(info);
}
void audio_eof_speech(const char *info) {
  Serial.print("eof_speech  ");
  Serial.println(info);
}
