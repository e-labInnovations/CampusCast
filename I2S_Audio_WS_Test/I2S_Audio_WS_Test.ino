#include "WiFi.h"
#include "Audio.h"
#include <WiFiManager.h>       // https://github.com/tzapu/WiFiManager
#include <WebSocketsClient.h>  //version 2.3.6

#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC 25

// const char* ssid = "e-labinnovations";
// const char* password = "PASSWORD";

Audio audio;
WiFiManager wm;
WebSocketsClient webSocket;

char *audioURL;
bool playAudio = false;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("CampusCast"); // anonymous ap
  res = wm.autoConnect("CampusCast", "password");  // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }

  // server address, port and URL
  webSocket.begin("192.168.29.69", 1880, "/");
  // event handler
  webSocket.onEvent(webSocketEvent);
  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocket.enableHeartbeat(15000, 3000, 2);

  // audio.connecttohost("http://vis.media-ice.musicradio.com/CapitalMP3");
  // audio.connecttohost("https://firebasestorage.googleapis.com/v0/b/campuscast-elabins.appspot.com/o/recording-test01.m4a?alt=media&token=2e25980f-1d7c-4675-9a52-67a0c65170fb");
}

void loop() {
  webSocket.loop();
  audio.loop();

  if (playAudio) {
    // audio.connecttohost(audioURL);
    webSocket.disconnect();
    audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Olsen-Banden.mp3");
    // audio.connecttospeech("New announcement received", "en");
    playAudio = false;
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
      webSocket.sendTXT("{ \"command\":\"connected\", \"classroom_code\": \"236\" }");
      audio.connecttospeech("Connected CampusCast server", "en");
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);

      playAudio = true;
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);

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