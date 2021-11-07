//модуль устранения дребезга кнопок
#define encopinB 33                 // контат кнопка левый энкодер
boolean encoderBtnState = false;            // Статус кнопки
uint16_t btLF = 1;                   // Фильтр для антидребезга
boolean encoderBtnPressed = false;          // Кнопка 1 отпущена

int buttonCount = 0;                 // Количество нажатий

void buttonsInit () {
  pinMode (encopinB, INPUT);        // кнопка левый энкодер
}

void refreshButtons () {

  if (!digitalRead(encopinB)) {    // Если кнопка левого энкодера нажата
    // если бит доехал до левого угла B1000000000000000
    if (btLF == 32768) encoderBtnState = true; else btLF <<= 1;
  } else {                       // Если кнопка 1отпущена
    // если бит доехал до правого угла B0000000000000001
    if (btLF == 1) encoderBtnState = false; else btLF >>= 1;
  }
}
