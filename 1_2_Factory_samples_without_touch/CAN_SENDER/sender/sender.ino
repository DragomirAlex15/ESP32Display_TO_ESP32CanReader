#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

uint8_t receiverMAC[] = { 0xA8, 0x42, 0xE3, 0xA8, 0xBA, 0x00 };
 

typedef struct {
  int apa;
  int ulei_p;
  int ulei_t;
  int incarcare;
} SensorData;

SensorData tx;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  esp_now_init();

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, receiverMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("Sender ESP-NOW gata.");
  Serial.println("Scrie: apa,ulei_p,ulei_t,incarcare");
  Serial.println("Ex: 90,4,105,80");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (sscanf(line.c_str(), "%d,%d,%d,%d",
               &tx.apa,
               &tx.ulei_p,
               &tx.ulei_t,
               &tx.incarcare) == 4) {

      esp_now_send(receiverMAC, (uint8_t*)&tx, sizeof(tx));
      Serial.println("Trimis ✔");
    } else {
      Serial.println("Format gresit!");
    }
  }
}
