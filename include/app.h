#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_PN532.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ESP32Encoder.h>
#include <Preferences.h>
#include "Arduino.h"
// #include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>

class App {
public:
  App();
  void Run();
private:
  void InitDisplay();
  void InitEncoder();
  void InitPN532();
  void displayWelcomeScreen(uint32_t delayMs);
  void loop();
  void startListeningToNFC();
  void handleCardDetected();
  void Menu();
  void handleButtons();
private:
  Adafruit_SSD1306 display;
  ESP32Encoder encoder;
  Adafruit_PN532 nfc;
  Preferences preferences;
  uint8_t irqCurr{0};
  uint8_t irqPrev{0};
  bool readerDisabled{false};
  uint64_t timeLastCardRead;
  uint8_t menuItem{0};
  uint8_t cardsCount{0};
  uint32_t newCard;
  uint32_t cards[6];
  uint8_t i;
  uint32_t t_value;
  char arrayOfChars[2];
};
