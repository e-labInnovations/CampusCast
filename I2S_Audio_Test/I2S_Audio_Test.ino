#include "WiFi.h"
#include "Audio.h"
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC 25

// const char* ssid = "e-labinnovations";
// const char* password = "PASSWORD";

Audio audio;
WiFiManager wm;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true); 

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

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);
  // audio.connecttohost("http://vis.media-ice.musicradio.com/CapitalMP3");
  audio.connecttohost("https://firebasestorage.googleapis.com/v0/b/campuscast-elabins.appspot.com/o/recording-test01.m4a?alt=media&token=2e25980f-1d7c-4675-9a52-67a0c65170fb");
}

void loop() {
  audio.loop();
}

// optional
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);
}
void audio_bitrate(const char *info){
    Serial.print("bitrate     ");Serial.println(info);
}
void audio_commercial(const char *info){  //duration in sec
    Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    Serial.print("lasthost    ");Serial.println(info);
}
void audio_eof_speech(const char *info){
    Serial.print("eof_speech  ");Serial.println(info);
}