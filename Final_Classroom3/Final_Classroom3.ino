#include <SPI.h>
#include <MFRC522.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= RFID =================
#define SS_PIN 5
#define RST_PIN 4
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ================= LORA =================
#define LORA_SS   27
#define LORA_RST  14
#define LORA_DIO0 26

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();

  // LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    lcd.clear();
    lcd.print("LoRa FAIL");
    while (true);
  }

  lcd.clear();
  lcd.print("Scan Card...");
}

void loop() {

  // wait for RFID
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // ===== READ UID =====
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.print("UID: ");
  Serial.println(uid);

  // ===== MAP UID → NAME + ROOM =====
  String name = "";
  String room = "";

  if (uid == "46D3106") {
    name = "C. Cabrera";
    room = "ROOM3";
  }
  else if (uid == "2EABD26") {
    name = "C. De Guzman";
    room = "ROOM3";
  }
  else if (uid == "29C1136") {
    name = "U. Millendrez";
    room = "ROOM3";
  }
  else if (uid == "F12CF6") {
    name = "N. Magnaye";
    room = "ROOM3";
  }
  else if (uid == "E636D26") {
    name = "G. Binay";
    room = "ROOM3";
  }
  else if (uid == "4CC5106") {
    name = "E. Rodriguez";
    room = "ROOM3";
  }
  else {
    name = "Unknown";
    room = "ROOM?";
  }

  // ===== LCD DISPLAY =====
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(room);
  lcd.setCursor(0, 1);
  lcd.print(name);

  // ===== LORA SEND =====
  String message = room + "|" + name;

  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  Serial.println("Sent: " + message);

  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan Card...");
}
