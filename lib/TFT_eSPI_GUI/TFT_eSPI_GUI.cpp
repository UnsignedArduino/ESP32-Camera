#include <Arduino.h>
#include <TFT_eSPI.h>
#include "TFT_eSPI_GUI.h"

// https://github.com/adafruit/Adafruit_Arcada/blob/master/Adafruit_Arcada_Alerts.cpp#L200
uint8_t TFT_eSPI_GUI_menu(TFT_eSPI tft, const char** menu, uint8_t menuCount,
                          Button upButton, Button downButton,
                          Button selectButton) {
  const uint8_t charWidth = 6;
  const uint8_t charHeight = 8;
  const uint16_t boxColor = TFT_WHITE;
  const uint16_t textColor = TFT_BLACK;
  const uint8_t topPadding = 8;
  const uint8_t rightPadding = 8;
  const uint8_t bottomPadding = 16;
  const uint8_t leftPadding = 8;

  uint16_t boxWidth = tft.width() - leftPadding - rightPadding;
  uint16_t boxHeight = tft.height() - topPadding - bottomPadding;
  uint16_t boxX = leftPadding;
  uint16_t boxY = topPadding;
  const uint8_t maxCharPerRow = boxWidth / charWidth;

  tft.fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1, boxColor);
  tft.drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1, textColor);

  const uint8_t fontX = boxX + charWidth / 2;
  const uint8_t fontY = boxY + charHeight;

  uint8_t selected = 0;

  while (true) {
    for (int i = 0; i < menuCount; i++) {
      if (i == selected) {
        tft.setTextColor(boxColor, textColor);
      } else {
        tft.setTextColor(textColor, boxColor);
      }
      tft.setCursor(fontX, fontY + charHeight * i);
      tft.print(" ");
      tft.print(menu[i]);
      for (int j = strlen(menu[i])+1; j < maxCharPerRow-1; j++) {
        tft.print(" ");
      }
    }

    while (true) {
      if (upButton.pressed()) {
        if (selected == 0) {
          selected = menuCount - 1;
        } else {
          selected--;
        }
        break;
      }
      if (downButton.pressed()) {
        if (selected == menuCount - 1) {
          selected = 0;
        } else {
          selected++;
        }
        break;
      }
      if (selectButton.pressed()) {
        return selected;
      }
    }
  }
}
