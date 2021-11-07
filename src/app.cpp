#include "app.h"
#include "buttons.h"

static constexpr uint32_t welcomeScreenDelay(2000);

static constexpr uint8_t SCREEN_WIDTH(128);
static constexpr uint8_t SCREEN_HEIGHT(64);
static constexpr uint8_t SCREEN_ADDRESS(0x3c);

static constexpr uint64_t DELAY_BETWEEN_CARDS = 2000;

static constexpr uint8_t MAX_CARDS_COUNT(5);

App::App()
  : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1)
  , nfc(23, 5) {
}

void App::Run() {
  preferences.begin("rfid", true);
  // preferences.begin("rfid", false);
  // preferences.clear();
  // preferences.end();
  // ESP.restart();
  InitDisplay();
  InitEncoder();
  InitPN532();
  displayWelcomeScreen(welcomeScreenDelay);
  startListeningToNFC();
  cardsCount = preferences.getUChar("cardsCount", 0);
  Serial.printf("Current cardsCount = %u\n", cardsCount);
  if (cardsCount > 0) {
    for (i=1;i<=cardsCount;i++) {
      itoa(i,arrayOfChars,10);
      cards[i] = preferences.getUInt(arrayOfChars);
      Serial.printf("Read card[%u] = %u\n", i, cards[i]);
    }
  }
  preferences.end();
  while(true) {
    loop();
  }
}

void App::startListeningToNFC() {
  // Reset our IRQ indicators
  irqPrev = 1;
  irqCurr = 1;

  Serial.println("Waiting for an ISO14443A Card ...");
  nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
}

void App::handleCardDetected() {
  uint8_t success = false;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};  // Buffer to store the returned UID
  uint8_t uidLength;  // Length of the UID (4 or 7 bytes depending on ISO14443A
                      // card type)

  // read the NFC tag's info
  success = nfc.readDetectedPassiveTargetID(uid, &uidLength);
  Serial.println(success ? "Read successful" : "Read failed (not a card?)");

  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);

    if (uidLength == 4) {
      // We probably have a Mifare Classic card ...
      uint32_t cardid = uid[0];
      cardid <<= 8;
      cardid |= uid[1];
      cardid <<= 8;
      cardid |= uid[2];
      cardid <<= 8;
      cardid |= uid[3];
      Serial.print("Seems to be a Mifare Classic card #");
      Serial.println(cardid);
      if (menuItem == 0) {
        newCard = cardid;
        menuItem = 100;
      }
      if (menuItem == 11) {
        newCard = cardid;
        menuItem = 12; //card detected
      }
    }
    Serial.println("");

  }
  timeLastCardRead = millis();

  // The reader will be enabled again after DELAY_BETWEEN_CARDS ms will pass.
  readerDisabled = true;
}

void App::loop() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  handleButtons();
  Menu();

  if (readerDisabled) {
    if (millis() - timeLastCardRead > DELAY_BETWEEN_CARDS) {
      readerDisabled = false;
      startListeningToNFC();
    }
  } else {
    irqCurr = digitalRead(23);

    // When the IRQ is pulled low - the reader has got something for us.
    if (irqCurr == LOW && irqPrev == HIGH) {
       Serial.println("Got NFC IRQ");  
       handleCardDetected(); 
    }
  
    irqPrev = irqCurr;
  }
}

void App::InitDisplay() {
  Serial.println("Init display");
  Wire1.setPins(18,19);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
}

void App::InitEncoder() {
  Serial.println("Init encoder");
  ESP32Encoder::useInternalWeakPullResistors = UP;
  pinMode(33, INPUT); // encoder button
  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH); // power 5V 
  encoder.attachSingleEdge(26, 25); // aPin = clk, bPin = dt
  encoder.setCount(0);
}

void App::InitPN532() {
  Serial.println("Init pn532");

  Wire.begin(-1, -1, 100000L);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(1, 1);
    display.print("Error init pn532");
    display.display();
    Serial.println("Error init pn532");
    while (1)
      ;  // halt
  }
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();
}

void App::displayWelcomeScreen(uint32_t delayMs) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(15, 24);
  display.println(F("Quadcode"));
  display.display();
  delay(delayMs);
}

void App::handleButtons() {
  refreshButtons();
  if (encoderBtnState == true) {          // Кнопка левого энкодера точно была нажата
    if (encoderBtnPressed == false) {
      // кнопка была отпущена после последнего нажатия
      Serial.println("Encoder button pressed");
      Serial.printf("Encoder value = %lli\n",encoder.getCount());
      Serial.printf("menuItem = %u\n",menuItem);
      switch (menuItem)
      {
        case 0:
          encoder.setCount(1);
          menuItem = encoder.getCount();
          break;
        case 1:
          if (cardsCount < MAX_CARDS_COUNT) {
            menuItem = 11;
          }
          break;
        case 2:
          menuItem = 21;
          break;
        case 3:
          menuItem = 0;
          displayWelcomeScreen(welcomeScreenDelay);
          break;
        case 11:
          menuItem = 1;
          break;
        case 12:
          preferences.begin("rfid", false);
          itoa(cardsCount+1,arrayOfChars,10);
          preferences.putUInt(arrayOfChars, newCard);
          Serial.println("preferences.putUInt newCard");
          t_value = preferences.getUInt(arrayOfChars);
          Serial.printf("ReadUInt newCard: %u\n", t_value);
          if (newCard == t_value ) {
            // write card to memory successful
            Serial.println("Increase cards count..");
            cards[cardsCount+1] = newCard;
            cardsCount += 1;
            preferences.putUChar("cardsCount",cardsCount);
            preferences.end();
            menuItem = 14;
          } else {
            // write card to memory failed
            Serial.println("Write card to memory failed");
          }
          break;
        case 13:
          menuItem = 1;
          break;
        case 21:
          menuItem = 2;
          break;
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
          // delete card (menuItem-21)
          preferences.begin("rfid", false);
          for (i=1; i<=cardsCount; i++) {
            if (i >= menuItem-21) {
              if (i<5) {
                cards[i] = cards[i+1];
                itoa(i+1,arrayOfChars,10);
                preferences.putUInt(arrayOfChars, cards[i+1]);
              }
            }
          }
          cardsCount -= 1;
          preferences.putUChar("cardsCount",cardsCount);
          preferences.end();
          menuItem = 2;
          break;
      }

      encoderBtnPressed = true;           // запомнили что учли это нажатие
    }
  } else {                         // кнопка отпущена
    encoderBtnPressed = false;
  }
  
  // LOGICS
  switch (menuItem)
  {
    case 0:
      break;
    case 1:
    case 2:
    case 3:
      menuItem = encoder.getCount();
      if (menuItem > 3) {
        encoder.setCount(3);
        menuItem = encoder.getCount();
      }
      if (menuItem < 1) {
        encoder.setCount(1);
        menuItem = encoder.getCount();
      }
      break;
    case 12:
    case 13:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
      if (encoder.getCount() > 13) encoder.setCount(13);
      if (encoder.getCount() < 12) encoder.setCount(12);
      menuItem = encoder.getCount();
      break;
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
      if (encoder.getCount() > 21+cardsCount) encoder.setCount(21+cardsCount);
      if (encoder.getCount() < 21) encoder.setCount(21);
      menuItem = encoder.getCount();
      break;
  }
}

void App::Menu() {
  if (menuItem > 0 && menuItem <=3) {
    // Serial.println("Draw menu");
    display.setTextSize(2);
    display.setTextColor(WHITE);

    display.setCursor(30, 0);
    display.println("MENU");
    //---------------------------------
    display.setTextSize(1);
    display.setCursor(10, 20);
    if (cardsCount < MAX_CARDS_COUNT) {
      display.println("Read card ->");
    } else {
      display.println("Read card");
    }

    display.setCursor(10, 30);
    display.print("View cards [");
    display.print(cardsCount);
    display.println("] ->");
    display.setCursor(10, 40);
    display.println("Exit");

    display.setCursor(2, (menuItem * 10) + 10);
    display.println(">");

    display.display();
  }

  if (menuItem >= 11 && menuItem <=19) {
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(10, 0);
    display.println("Reading card...");
    if (menuItem == 12 || menuItem == 13) {
      display.setCursor(10, 10);
      display.println("Card detected.");
      
      display.setCursor(10, 20);
      display.print("#");
      display.setCursor(20, 20);
      display.println(newCard);
      
      display.setCursor(10, 30);
      display.println("Store?");
      display.setCursor(30, 40);
      display.println("Yes");
      display.setCursor(30, 50);
      display.println("No");

      display.setCursor(22, ((menuItem-11) * 10) + 30);
      display.println(">"); 
    }
    if (menuItem == 14) {
      Serial.println("Draw menuItem=14");
      display.setCursor(5, 20);
      display.println("Card has been stored"); 
    }
    
    display.display();

    if (menuItem == 14) {
      delay(2000);
      menuItem = 1;
    }
  }

  if (menuItem >= 21 && menuItem <=29) {
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(30, 0);
    display.println("Cards:");
    if (cardsCount > 0) {
      for (i=0;i<cardsCount;i++) {
        display.setCursor(10, i*10+10);
        display.printf("%d. %u", i+1, cards[i+1]);
      }
      if (menuItem > 21) {
        display.setCursor(2, ((menuItem-22) * 10) + 10);
        display.println(">");
      } 
    }
    display.display();
  }
  
  if (menuItem == 100) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(30, 0);
    display.println("Card detected");

    if (cardsCount > 0) {
      for (i=0;i<cardsCount;i++) {
        if (newCard == cards[i+1]) {
          display.setCursor(40, 20);
          display.printf("#%d", i+1);
          display.display();
          break;
        }
      }
      if (i==cardsCount) {
        display.setCursor(20, 20);
        display.printf("Unknown card");
      }

      display.display();
    } else {
      display.setCursor(40, 20);
      display.printf("Unknown card");

      display.display();
    }
    menuItem = 0;
    delay(2000);
    displayWelcomeScreen(500);
  }
}