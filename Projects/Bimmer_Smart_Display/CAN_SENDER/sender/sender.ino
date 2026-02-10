#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

uint8_t receiverMAC[] = { 0xA8, 0x42, 0xE3, 0xA8, 0xBA, 0x00 };

typedef struct {
    int incarcare;
    int temp_apa;
    int temp_ulei;
    int pres_ulei;
    int lambda;
    int intake;
    int rpm;
    int maf;
    int map;
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
  Serial.println("Introdu 9 valori separate prin virgula:");
  Serial.println("incarcare,temp_apa,temp_ulei,pres_ulei,lambda,intake,rpm,maf,map");
  Serial.println("Exemplu:");
  Serial.println("80,90,105,3,980,32,2500,18,102");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (sscanf(line.c_str(), "%d,%d,%d,%d,%d,%d,%d,%d,%d",
               &tx.incarcare,
               &tx.temp_apa,
               &tx.temp_ulei,
               &tx.pres_ulei,
               &tx.lambda,
               &tx.intake,
               &tx.rpm,
               &tx.maf,
               &tx.map) == 9) {

      esp_now_send(receiverMAC, (uint8_t*)&tx, sizeof(tx));
      Serial.println("Trimis ✔");

    } else {
      Serial.println("Format gresit! Ai nevoie de 9 valori separate prin virgula.");
    }
  }
}
