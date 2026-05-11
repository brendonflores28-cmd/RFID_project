#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <time.h>

#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ================= WIFI =================
#define WIFI_SSID "Galaxy"
#define WIFI_PASSWORD "mikayyyyyy"

// ================= FIREBASE =================
#define API_KEY "AIzaSyDH_licIPGRqWHHIGF4DEzJqeshH-gVPPc"
#define PROJECT_ID "flores-integ2f3-eef2b"

// ================= LORA =================
#define SS   27
#define RST  14
#define DIO0 26

// ================= BUTTON =================
#define BTN 32

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ================= FIREBASE OBJECTS =================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// ================= STORAGE =================
String rooms[10];
String names[10];
int count = 0;

// ================= SCROLL =================
int displayIndex = 0;
bool lastBtnState = HIGH;

// ================= TIME =================
String getTimeNow() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "NoTime";

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.print("Connecting WiFi");

  // ===== WIFI =====
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected");

  // ===== FIREBASE =====
  config.api_key = API_KEY;

  if (Firebase.signUp(&config, &auth, "", "")) {
    signupOK = true;
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // ===== TIME =====
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  // ===== LORA =====
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    lcd.clear();
    lcd.print("LoRa FAILED!");
    while (true);
  }

  lcd.clear();
  lcd.print("Gateway Ready");
  delay(2000);
  lcd.clear();
}

void loop() {

  // ================= RECEIVE LORA =================
  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    String incoming = "";

    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    Serial.println("Received: " + incoming);

    int sep = incoming.indexOf('|');

    if (sep != -1) {

      String room = incoming.substring(0, sep);
      String name = incoming.substring(sep + 1);

      // ===== STORE =====
      bool exists = false;

      for (int i = 0; i < count; i++) {
        if (rooms[i] == room) {
          names[i] = name;
          exists = true;
          break;
        }
      }

      if (!exists && count < 10) {
        rooms[count] = room;
        names[count] = name;
        count++;
      }

      displayIndex = 0;
      updateDisplay();

      // ===== SEND TO FIREBASE =====
      if (signupOK) {
        FirebaseJson content;

        content.set("fields/room/stringValue", room);
        content.set("fields/name/stringValue", name);
        content.set("fields/time/stringValue", getTimeNow());

        String path = "rfid_logs/" + String(millis());

        if (Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, "", path.c_str(), content.raw())) {
          Serial.println("✅ Sent to Firebase");
        } else {
          Serial.println("❌ Error: " + fbdo.errorReason());
        }
      }
    }
  }

  // ================= BUTTON =================
  bool btnState = digitalRead(BTN);

  if (btnState == LOW && lastBtnState == HIGH) {
    if (count > 0) {
      displayIndex++;
      if (displayIndex >= count) displayIndex = 0;

      updateDisplay();
    }
    delay(200);
  }

  lastBtnState = btnState;
}

// ================= DISPLAY =================
void updateDisplay() {
  lcd.clear();

  for (int i = 0; i < 4; i++) {
    int idx = displayIndex + i;

    if (idx >= count) break;

    lcd.setCursor(0, i);
    lcd.print(rooms[idx]);
    lcd.print(" ");
    lcd.print(names[idx]);
  }
}
