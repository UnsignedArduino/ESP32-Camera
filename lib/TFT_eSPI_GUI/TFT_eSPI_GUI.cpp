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

  const uint8_t maxEntryPerPage = 10;
  const uint16_t timePerChar = 200;
  const uint8_t startEndPauseTicks = 4;
  const uint16_t holdToAccelTime = 500;
  const uint16_t moveThrottleTime = 50;

  uint32_t selectedTime = millis();
  uint32_t lastPressTime = millis();
  uint32_t pressTime = 0;
  uint32_t lastMovedTime = millis();
  const bool showScrollbar = menuCount > maxEntryPerPage;
  const uint8_t maxCharPerRow =
    (boxWidth / charWidth) - (showScrollbar ? 1 : 0);

  const uint8_t fontX = boxX + charWidth / 2;
  uint8_t fontY = boxY + charHeight;

  tft.fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1, boxColor);
  tft.drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1, textColor);

  tft.setTextColor(textColor, boxColor);
  tft.setCursor(fontX, fontY);
  tft.print(" ");
  tft.print(title);

  fontY += charHeight * 1.5;

  const uint8_t scrollBarX = boxX + boxWidth - charWidth;
  const uint8_t scrollBarY = fontY;
  const uint8_t scrollBarWidth = charWidth / 2;
  const uint8_t scrollBarHeight = maxEntryPerPage * charHeight;

  uint8_t offset = 0;
  uint8_t selected = 0;

  uint8_t offsetPauseTicks = startEndPauseTicks;
  uint8_t selectedCharOffset = 0;
  bool resetAfterPause = false;

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
      if (i == selected) {
        for (uint8_t j = 1;
             j < min((size_t)maxCharPerRow - 2, strlen(menu[i]) + 1); j++) {
          tft.print(menu[i][j - 1 + selectedCharOffset]);
        }
      } else {
        for (uint8_t j = 1;
             j < min((size_t)maxCharPerRow - 2, strlen(menu[i]) + 1); j++) {
          tft.print(menu[i][j - 1]);
        }
      }
      if (strlen(menu[i]) > maxCharPerRow - 3) {
        tft.print(" ");
      }
      for (int j = strlen(menu[i]); j < maxCharPerRow - 2; j++) {
        tft.print(" ");
      }
    }

    if (showScrollbar) {
      const uint8_t startY =
        map(min((int16_t)offset, (int16_t)(menuCount - maxEntryPerPage)), 0,
            menuCount, scrollBarY, scrollBarY + scrollBarHeight);
      const uint8_t barHeight =
        map(maxEntryPerPage, 0, menuCount, 0, scrollBarHeight);

      tft.fillRect(scrollBarX, scrollBarY, scrollBarWidth, scrollBarHeight,
                   boxColor);
      tft.fillRect(scrollBarX, startY, scrollBarWidth, barHeight, textColor);
    }

    while (millis() - lastMovedTime < moveThrottleTime) {
      delay(1);
    }

    while (true) {
      if (!upButton.read() || !downButton.read()) {
        if (lastPressTime > 0) {
          pressTime += millis() - lastPressTime;
        }
        lastPressTime = millis();
      } else {
        pressTime = 0;
        lastPressTime = 0;
      }
      if (upButton.pressed() ||
          (!upButton.read() && pressTime > holdToAccelTime)) {
        if (selected == 0) {
          selected = menuCount - 1;
        } else {
          selected--;
        }
        selectedCharOffset = 0;
        selectedTime = millis();
        break;
      }
      if (downButton.pressed() ||
          (!downButton.read() && pressTime > holdToAccelTime)) {
        if (selected == menuCount - 1) {
          selected = 0;
        } else {
          selected++;
        }
        selectedCharOffset = 0;
        selectedTime = millis();
        break;
      }
      if (selectButton.pressed()) {
        return selected;
      }
      if (millis() - selectedTime > timePerChar &&
          strlen(menu[selected]) > maxCharPerRow - 3) {
        selectedTime = millis();
        if (offsetPauseTicks > 0) {
          offsetPauseTicks--;
        } else if (resetAfterPause) {
          resetAfterPause = false;
          selectedCharOffset = 0;
          offsetPauseTicks = startEndPauseTicks;
          break;
        } else {
          selectedCharOffset++;
          if (selectedCharOffset + maxCharPerRow >=
              strlen(menu[selected]) + 3) {
            offsetPauseTicks = startEndPauseTicks;
            resetAfterPause = true;
          }
          break;
        }
      }
    }

    if (selected >= offset + maxEntryPerPage) {
      offset += selected - (offset + maxEntryPerPage) + 1;
    } else if (selected < offset) {
      offset = selected;
    }

    lastMovedTime = millis();
  }
}

bool TFT_eSPI_GUI_file_explorer(TFT_eSPI tft, SdFs& sd, char* startDirectory,
                                Button upButton, Button downButton,
                                Button selectButton, Button shutterButton,
                                char* result, size_t resultSize) {
  const uint8_t menuCount = 20;
  const char* menu[menuCount] = {"Super duper long menu item",
                                 "Perfect fit menu item",
                                 "Actual fit menu item",
                                 "Menu item 4",
                                 "Menu item 5",
                                 "Menu item 6",
                                 "Menu item 7",
                                 "Menu item 8",
                                 "Menu item 9",
                                 "Menu item 10",
                                 "Menu item 11",
                                 "Menu item 12",
                                 "Menu item 13",
                                 "Menu item 14",
                                 "Menu item 15",
                                 "Menu item 16",
                                 "Menu item 17",
                                 "Menu item 18",
                                 "Menu item 19",
                                 "Menu item 20"};

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

  const uint8_t maxEntryPerPage = 8;
  const uint16_t timePerChar = 200;
  const uint8_t startEndPauseTicks = 4;
  const uint16_t holdToAccelTime = 500;
  const uint16_t moveThrottleTime = 50;

  uint32_t selectedTime = millis();
  uint32_t lastPressTime = millis();
  uint32_t pressTime = 0;
  uint32_t lastMovedTime = millis();
  const bool showScrollbar = menuCount > maxEntryPerPage;
  const uint8_t maxCharPerRow =
    (boxWidth / charWidth) - (showScrollbar ? 1 : 0);

  const uint8_t fontX = boxX + charWidth / 2;
  uint8_t fontY = boxY + charHeight;

  tft.fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1, boxColor);
  tft.drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1, textColor);

  tft.setTextColor(textColor, boxColor);
  tft.setCursor(fontX, fontY);
  tft.print(" File explorer");

  fontY += charHeight * 1.5;

  tft.setCursor(fontX, fontY);
  tft.print(" ");
  tft.print(startDirectory);

  fontY += charHeight * 1.5;

  const uint8_t scrollBarX = boxX + boxWidth - charWidth;
  const uint8_t scrollBarY = fontY;
  const uint8_t scrollBarWidth = charWidth / 2;
  const uint8_t scrollBarHeight = maxEntryPerPage * charHeight;

  uint8_t offset = 0;
  uint8_t selected = 0;

  uint8_t offsetPauseTicks = startEndPauseTicks;
  uint8_t selectedCharOffset = 0;
  bool resetAfterPause = false;

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
      if (i == selected) {
        for (uint8_t j = 1;
             j < min((size_t)maxCharPerRow - 2, strlen(menu[i]) + 1); j++) {
          tft.print(menu[i][j - 1 + selectedCharOffset]);
        }
      } else {
        for (uint8_t j = 1;
             j < min((size_t)maxCharPerRow - 2, strlen(menu[i]) + 1); j++) {
          tft.print(menu[i][j - 1]);
        }
      }
      if (strlen(menu[i]) > maxCharPerRow - 3) {
        tft.print(" ");
      }
      for (int j = strlen(menu[i]) + 1; j < maxCharPerRow - 1; j++) {
        tft.print(" ");
      }
    }

    if (showScrollbar) {
      const uint8_t startY =
        map(min((int16_t)offset, (int16_t)(menuCount - maxEntryPerPage)), 0,
            menuCount, scrollBarY, scrollBarY + scrollBarHeight);
      const uint8_t barHeight =
        map(maxEntryPerPage, 0, menuCount, 0, scrollBarHeight);

      tft.fillRect(scrollBarX, scrollBarY, scrollBarWidth, scrollBarHeight,
                   boxColor);
      tft.fillRect(scrollBarX, startY, scrollBarWidth, barHeight, textColor);
    }

    while (millis() - lastMovedTime < moveThrottleTime) {
      delay(1);
    }

    while (true) {
      if (!upButton.read() || !downButton.read()) {
        if (lastPressTime > 0) {
          pressTime += millis() - lastPressTime;
        }
        lastPressTime = millis();
      } else {
        pressTime = 0;
        lastPressTime = 0;
      }
      if (upButton.pressed() ||
          (!upButton.read() && pressTime > holdToAccelTime)) {
        if (selected == 0) {
          selected = menuCount - 1;
        } else {
          selected--;
        }
        selectedCharOffset = 0;
        selectedTime = millis();
        break;
      }
      if (downButton.pressed() ||
          (!downButton.read() && pressTime > holdToAccelTime)) {
        if (selected == menuCount - 1) {
          selected = 0;
        } else {
          selected++;
        }
        selectedCharOffset = 0;
        selectedTime = millis();
        break;
      }
      if (selectButton.pressed()) {
        return true;
      }
      if (millis() - selectedTime > timePerChar &&
          strlen(menu[selected]) > maxCharPerRow - 3) {
        selectedTime = millis();
        if (offsetPauseTicks > 0) {
          offsetPauseTicks--;
        } else if (resetAfterPause) {
          resetAfterPause = false;
          selectedCharOffset = 0;
          offsetPauseTicks = startEndPauseTicks;
          break;
        } else {
          selectedCharOffset++;
          if (selectedCharOffset + maxCharPerRow >=
              strlen(menu[selected]) + 3) {
            offsetPauseTicks = startEndPauseTicks;
            resetAfterPause = true;
          }
          break;
        }
      }
    }
    if (selected >= offset + maxEntryPerPage) {
      offset += selected - (offset + maxEntryPerPage) + 1;
    } else if (selected < offset) {
      offset = selected;
    }

    lastMovedTime = millis();
  }
}
