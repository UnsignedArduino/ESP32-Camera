#include <Arduino.h>
#include <TFT_eSPI.h>
#include "TFT_eSPI_GUI.h"

// https://github.com/adafruit/Adafruit_Arcada/blob/master/Adafruit_Arcada_Alerts.cpp#L200
uint8_t TFT_eSPI_GUI_menu(TFT_eSPI tft, const char* title, const char** menu,
                          uint8_t menuCount, Button upButton, Button downButton,
                          Button selectButton) {
  const uint8_t charWidth = 6;
  const uint8_t charHeight = 8;
  const uint16_t boxColor = TFT_WHITE;
  const uint16_t textColor = TFT_BLACK;
  const uint8_t topPadding = 8;
  const uint8_t rightPadding = 8;
  const uint8_t bottomPadding = 16;
  const uint8_t leftPadding = 8;

  const uint16_t boxWidth = tft.width() - leftPadding - rightPadding;
  const uint16_t boxHeight = tft.height() - topPadding - bottomPadding;
  const uint16_t boxX = leftPadding;
  const uint16_t boxY = topPadding;
  const uint8_t maxCharPerRow = boxWidth / charWidth;

  tft.fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1, boxColor);
  tft.drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1, textColor);

  const uint8_t fontX = boxX + charWidth / 2;
  uint8_t fontY = boxY + charHeight;

  tft.setTextColor(textColor, boxColor);
  tft.setCursor(fontX, fontY);
  tft.print(" ");
  tft.print(title);

  fontY += charHeight * 1.5;

  const uint8_t maxEntryPerPage = 10;
  uint8_t offset = 0;
  uint8_t selected = 0;

  while (true) {
    for (int i = offset;
         i < min((uint16_t)menuCount, (uint16_t)(offset + maxEntryPerPage));
         i++) {
      if (i == selected) {
        tft.setTextColor(boxColor, textColor);
      } else {
        tft.setTextColor(textColor, boxColor);
      }
      tft.setCursor(fontX, fontY + charHeight * (i - offset));
      tft.print(" ");
      tft.print(menu[i]);
      for (int j = strlen(menu[i]) + 1; j < maxCharPerRow - 1; j++) {
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

    if (selected >= offset + maxEntryPerPage) {
      offset += selected - (offset + maxEntryPerPage) + 1;
    } else if (selected < offset) {
      offset = selected;
    }
  }
}

bool TFT_eSPI_GUI_file_explorer(TFT_eSPI tft, SdFs& sd, char* startDirectory,
                                Button upButton, Button downButton,
                                Button selectButton, Button shutterButton,
                                char* result, size_t resultSize) {
  const uint8_t charWidth = 6;
  const uint8_t charHeight = 8;
  const uint16_t boxColor = TFT_WHITE;
  const uint16_t textColor = TFT_BLACK;
  const uint8_t topPadding = 8;
  const uint8_t rightPadding = 8;
  const uint8_t bottomPadding = 16;
  const uint8_t leftPadding = 8;

  const uint16_t boxWidth = tft.width() - leftPadding - rightPadding;
  const uint16_t boxHeight = tft.height() - topPadding - bottomPadding;
  const uint16_t boxX = leftPadding;
  const uint16_t boxY = topPadding;
  const uint8_t maxCharPerRow = boxWidth / charWidth;

  tft.fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1, boxColor);
  tft.drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1, textColor);

  const uint8_t fontX = boxX + charWidth / 2;
  uint8_t fontY = boxY + charHeight;

  tft.setTextColor(textColor, boxColor);
  tft.setCursor(fontX, fontY);
  tft.print(" File explorer");

  fontY += charHeight * 1.5;

  tft.setCursor(fontX, fontY);
  tft.print(" ");
  tft.print(startDirectory);

  fontY += charHeight * 1.5;

  const uint8_t maxEntryPerPage = 8;
  uint8_t offset = 0;
  uint8_t selected = 0;

  const uint8_t menuCount = 20;
  const char* menu[menuCount] = {
    "Menu item 1",  "Menu item 2",  "Menu item 3",  "Menu item 4",
    "Menu item 5",  "Menu item 6",  "Menu item 7",  "Menu item 8",
    "Menu item 9",  "Menu item 10", "Menu item 11", "Menu item 12",
    "Menu item 13", "Menu item 14", "Menu item 15", "Menu item 16",
    "Menu item 17", "Menu item 18", "Menu item 19", "Menu item 20"};

  while (true) {
    for (int i = offset;
         i < min((uint16_t)menuCount, (uint16_t)(offset + maxEntryPerPage));
         i++) {
      if (i == selected) {
        tft.setTextColor(boxColor, textColor);
      } else {
        tft.setTextColor(textColor, boxColor);
      }
      tft.setCursor(fontX, fontY + charHeight * (i - offset));
      tft.print(" ");
      tft.print(menu[i]);
      for (int j = strlen(menu[i]) + 1; j < maxCharPerRow - 1; j++) {
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
        return true;
      }
    }

    if (selected >= offset + maxEntryPerPage) {
      offset += selected - (offset + maxEntryPerPage) + 1;
    } else if (selected < offset) {
      offset = selected;
    }
  }
}
