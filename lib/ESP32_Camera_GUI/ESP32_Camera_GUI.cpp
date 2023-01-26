#include <Arduino.h>
#include "ESP32_Camera_GUI.h"

bool ESP32CameraGUI::begin(TFT_eSPI* tft, SdFs* sd, RTC_DS3231* rtc,
                           Button* upButton, Button* downButton,
                           Button* selectButton, Button* shutterButton,
                           uint8_t battPin) {
  this->tft = tft;
  this->sd = sd;
  this->rtc = rtc;
  this->upButton = upButton;
  this->downButton = downButton;
  this->selectButton = selectButton;
  this->shutterButton = shutterButton;
  this->battPin = battPin;
  pinMode(this->battPin, INPUT);
  return true;
}

// https://github.com/adafruit/Adafruit_Arcada/blob/master/Adafruit_Arcada_Alerts.cpp#L200
uint8_t ESP32CameraGUI::menu(const char* title, const char** menu,
                             uint8_t menuCount, uint8_t startingSelected) {
  const uint8_t charWidth = 6;
  const uint8_t charHeight = 8;
  const uint16_t boxColor = TFT_WHITE;
  const uint16_t textColor = TFT_BLACK;
  const uint8_t topPadding = 8;
  const uint8_t rightPadding = 8;
  const uint8_t bottomPadding = 16;
  const uint8_t leftPadding = 8;

  const uint16_t boxWidth = this->tft->width() - leftPadding - rightPadding;
  const uint16_t boxHeight = this->tft->height() - topPadding - bottomPadding;
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
  const uint8_t maxCharPerRow = (boxWidth / charWidth) -
                                (showScrollbar ? 1 : 0) -
                                (startingSelected != 0xFF ? 2 : 0);

  const uint8_t fontX = boxX + charWidth / 2;
  uint8_t fontY = boxY + charHeight;

  this->tft->fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1,
                           boxColor);
  this->tft->drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1,
                           textColor);

  this->tft->setTextColor(textColor, boxColor);
  this->tft->setCursor(fontX, fontY);
  this->tft->print(" ");
  this->tft->print(title);

  fontY += charHeight * 1.5;

  const uint8_t scrollBarX = boxX + boxWidth - charWidth;
  const uint8_t scrollBarY = fontY;
  const uint8_t scrollBarWidth = charWidth / 2;
  const uint8_t scrollBarHeight = maxEntryPerPage * charHeight;

  uint8_t offset = 0;
  uint8_t selected = 0;
  if (startingSelected != 0xFF) {
    selected = startingSelected;
  }

  uint8_t offsetPauseTicks = startEndPauseTicks;
  uint8_t selectedCharOffset = 0;
  bool resetAfterPause = false;

  while (true) {
    for (int i = offset;
         i < min((uint16_t)menuCount, (uint16_t)(offset + maxEntryPerPage));
         i++) {
      if (i == selected) {
        this->tft->setTextColor(boxColor, textColor);
      } else {
        this->tft->setTextColor(textColor, boxColor);
      }
      this->tft->setCursor(fontX, fontY + charHeight * (i - offset));
      if (startingSelected != 0xFF) {
        if (i == startingSelected) {
          this->tft->print(" > ");
        } else {
          this->tft->print("   ");
        }
      } else {
        this->tft->print(" ");
      }
      if (i == selected) {
        for (uint8_t j = 1;
             j < min((size_t)maxCharPerRow - 2, strlen(menu[i]) + 1); j++) {
          this->tft->print(menu[i][j - 1 + selectedCharOffset]);
        }
      } else {
        for (uint8_t j = 1;
             j < min((size_t)maxCharPerRow - 2, strlen(menu[i]) + 1); j++) {
          this->tft->print(menu[i][j - 1]);
        }
      }
      if (strlen(menu[i]) > maxCharPerRow - 3) {
        this->tft->print(" ");
      }
      for (int j = strlen(menu[i]); j < maxCharPerRow - 2; j++) {
        this->tft->print(" ");
      }
    }

    if (showScrollbar) {
      const uint8_t startY =
        map(min((int16_t)offset, (int16_t)(menuCount - maxEntryPerPage)), 0,
            menuCount, scrollBarY, scrollBarY + scrollBarHeight);
      const uint8_t barHeight =
        map(maxEntryPerPage, 0, menuCount, 0, scrollBarHeight);

      this->tft->fillRect(scrollBarX, scrollBarY, scrollBarWidth,
                          scrollBarHeight, boxColor);
      this->tft->fillRect(scrollBarX, startY, scrollBarWidth, barHeight,
                          textColor);
    }

    this->drawBottomToolbar();

    while (millis() - lastMovedTime < moveThrottleTime) {
      delay(1);
    }

    while (true) {
      this->drawBottomToolbar();

      if (!this->upButton->read() || !this->downButton->read()) {
        if (lastPressTime > 0) {
          pressTime += millis() - lastPressTime;
        }
        lastPressTime = millis();
      } else {
        pressTime = 0;
        lastPressTime = 0;
      }
      if (this->upButton->pressed() ||
          (!this->upButton->read() && pressTime > holdToAccelTime)) {
        if (selected == 0) {
          selected = menuCount - 1;
        } else {
          selected--;
        }
        selectedCharOffset = 0;
        selectedTime = millis();
        break;
      }
      if (this->downButton->pressed() ||
          (!this->downButton->read() && pressTime > holdToAccelTime)) {
        if (selected == menuCount - 1) {
          selected = 0;
        } else {
          selected++;
        }
        selectedCharOffset = 0;
        selectedTime = millis();
        break;
      }
      if (this->selectButton->pressed()) {
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

bool ESP32CameraGUI::fileExplorer(const char* startDirectory, char* result,
                                  size_t resultSize) {
  uint32_t fileCount;

  FsFile dir;
  FsFile file;

  const size_t MAX_PATH_SIZE = 255;
  char currentDirectory[MAX_PATH_SIZE];
  strncpy(currentDirectory, startDirectory, MAX_PATH_SIZE);
  char selectedPath[MAX_PATH_SIZE];

  const uint8_t charWidth = 6;
  const uint8_t charHeight = 8;
  const uint16_t boxColor = TFT_WHITE;
  const uint16_t textColor = TFT_BLACK;
  const uint8_t topPadding = 8;
  const uint8_t rightPadding = 8;
  const uint8_t bottomPadding = 16;
  const uint8_t leftPadding = 8;

  const uint16_t boxWidth = this->tft->width() - leftPadding - rightPadding;
  const uint16_t boxHeight = this->tft->height() - topPadding - bottomPadding;
  const uint16_t boxX = leftPadding;
  const uint16_t boxY = topPadding;

  const uint8_t maxEntryPerPage = 8;
  const uint16_t timePerChar = 200;
  const uint8_t startEndPauseTicks = 4;
  const uint16_t holdToAccelTime = 500;
  const uint16_t moveThrottleTime = 50;

  const uint8_t extraOptionsCount = 2;
  const char* extraOptions[extraOptionsCount] = {"Exit", "Up a folder"};

  while (true) {
    char menuEntries[maxEntryPerPage][MAX_PATH_SIZE];
    uint32_t menuEntryOffset = -1;

    // dir = sd.open(currentDirectory, O_RDONLY);
    if (!this->getFileCount(currentDirectory, fileCount)) {
      return false;
    }

    Serial.printf("%lu files in %s\n", fileCount, currentDirectory);

    fileCount += extraOptionsCount;

    uint32_t selectedTime = millis();
    uint32_t lastPressTime = millis();
    uint32_t pressTime = 0;
    uint32_t lastMovedTime = millis();
    const bool showScrollbar = fileCount > maxEntryPerPage;
    const uint8_t maxCharPerRow =
      (boxWidth / charWidth) - (showScrollbar ? 1 : 0);
    const uint8_t maxCharInTitle = boxWidth / charWidth - 3;

    const uint8_t fontX = boxX + charWidth / 2;
    uint8_t fontY = boxY + charHeight;

    this->tft->fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1,
                             boxColor);
    this->tft->drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1,
                             textColor);

    this->tft->setTextColor(textColor, boxColor);
    this->tft->setCursor(fontX, fontY);
    this->tft->print(" File explorer");

    fontY += charHeight * 1.5;

    const uint16_t titleX = fontX;
    const uint16_t titleY = fontY;

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

    uint32_t lastTitleShiftTime = millis();
    uint8_t titleOffsetPauseTicks = startEndPauseTicks;
    uint8_t titleSelectedCharOffset = 0;
    bool titleResetAfterPause = false;

    bool needToCompleteRedraw = false;

    while (!needToCompleteRedraw) {
      if (menuEntryOffset != offset) {
        menuEntryOffset = offset;

        for (uint32_t i = offset;
             i < min((uint16_t)fileCount, (uint16_t)(offset + maxEntryPerPage));
             i++) {
          if (i < extraOptionsCount) {
            strncpy(menuEntries[i - offset], extraOptions[i], MAX_PATH_SIZE);
          } else {
            strncpy(menuEntries[i - offset], "Could not read file!",
                    MAX_PATH_SIZE);
            this->getFileNameFromIndex(currentDirectory, i - extraOptionsCount,
                                       menuEntries[i - offset], MAX_PATH_SIZE);
            // Serial.printf("File at index %d (menu entry %d) is \"%s\"\n",
            //               i - extraOptionsCount, i, menuEntries[i - offset]);
          }
        }
      }

      this->tft->setTextColor(textColor, boxColor);
      this->tft->setCursor(titleX, titleY);
      this->tft->print(" ");
      // Serial.printf("Title: %s\n", currentDirectory);
      for (uint8_t i = 1;
           i < min((size_t)maxCharInTitle, strlen(currentDirectory) + 1); i++) {
        this->tft->print(currentDirectory[i - 1 + titleSelectedCharOffset]);
        // Serial.print(currentDirectory[i - 1 + titleSelectedCharOffset]);
      }
      // Serial.println();

      for (uint32_t i = offset;
           i < min((uint16_t)fileCount, (uint16_t)(offset + maxEntryPerPage));
           i++) {
        char* current = menuEntries[i - offset];

        if (i == selected) {
          this->tft->setTextColor(boxColor, textColor);
          strncpy(selectedPath, current, MAX_PATH_SIZE);
          // Serial.printf("Selected file: \"%s\"\n", selectedPath);
        } else {
          this->tft->setTextColor(textColor, boxColor);
        }
        this->tft->setCursor(fontX, fontY + charHeight * (i - offset));
        this->tft->print(" ");
        if (i == selected) {
          for (uint8_t j = 1;
               j < min((size_t)maxCharPerRow - 2, strlen(current) + 1); j++) {
            this->tft->print(current[j - 1 + selectedCharOffset]);
          }
        } else {
          for (uint8_t j = 1;
               j < min((size_t)maxCharPerRow - 2, strlen(current) + 1); j++) {
            this->tft->print(current[j - 1]);
          }
        }
        if (strlen(current) > maxCharPerRow - 3) {
          this->tft->print(" ");
        }
        for (int j = strlen(current) + 1; j < maxCharPerRow - 1; j++) {
          this->tft->print(" ");
        }
      }

      if (showScrollbar) {
        const uint8_t startY =
          map(min((int16_t)offset, (int16_t)(fileCount - maxEntryPerPage)), 0,
              fileCount, scrollBarY, scrollBarY + scrollBarHeight);
        const uint8_t barHeight =
          map(maxEntryPerPage, 0, fileCount, 0, scrollBarHeight);

        this->tft->fillRect(scrollBarX, scrollBarY, scrollBarWidth,
                            scrollBarHeight, boxColor);
        this->tft->fillRect(scrollBarX, startY, scrollBarWidth, barHeight,
                            textColor);
      }

      this->drawBottomToolbar();

      while (millis() - lastMovedTime < moveThrottleTime) {
        delay(1);
      }

      while (true) {
        this->drawBottomToolbar();

        if (!this->upButton->read() || !this->downButton->read()) {
          if (lastPressTime > 0) {
            pressTime += millis() - lastPressTime;
          }
          lastPressTime = millis();
        } else {
          pressTime = 0;
          lastPressTime = 0;
        }
        if (this->upButton->pressed() ||
            (!this->upButton->read() && pressTime > holdToAccelTime)) {
          if (selected == 0) {
            selected = fileCount - 1;
          } else {
            selected--;
          }
          selectedCharOffset = 0;
          selectedTime = millis();
          break;
        }
        if (this->downButton->pressed() ||
            (!this->downButton->read() && pressTime > holdToAccelTime)) {
          if (selected == fileCount - 1) {
            selected = 0;
          } else {
            selected++;
          }
          selectedCharOffset = 0;
          selectedTime = millis();
          break;
        }
        if (this->selectButton->pressed()) {
          if (selected < extraOptionsCount) {
            switch (selected) {
              default:
              case 0: {
                return false;
              }
              case 1: {
                char* last = strrchr(currentDirectory, '/');
                if (last != 0) {
                  last[1] = '\0';
                }
                if (currentDirectory[strlen(currentDirectory) - 1] == '/' &&
                    strlen(currentDirectory) > 1) {
                  currentDirectory[strlen(currentDirectory) - 1] = '\0';
                }
                Serial.printf("chdir to .. (%s)\n", currentDirectory);
                needToCompleteRedraw = true;
                break;
              }
            }
            break;
          } else if (selectedPath[strlen(selectedPath) - 1] == '/') {
            if (currentDirectory[strlen(currentDirectory) - 1] == '/') {
              currentDirectory[strlen(currentDirectory) - 1] = '\0';
            }
            if (selectedPath[0] == '/') {
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE);
            } else {
              strncat(currentDirectory, "/", MAX_PATH_SIZE);
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE);
            }
            if (currentDirectory[strlen(currentDirectory) - 1] == '/') {
              currentDirectory[strlen(currentDirectory) - 1] = '\0';
            }
            Serial.printf("chdir to %s (%s)\n", selectedPath, currentDirectory);
            needToCompleteRedraw = true;
            break;
          } else {
            if (currentDirectory[strlen(currentDirectory) - 1] == '/') {
              currentDirectory[strlen(currentDirectory) - 1] = '\0';
            }
            if (selectedPath[0] == '/') {
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE);
            } else {
              strncat(currentDirectory, "/", MAX_PATH_SIZE);
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE);
            }
            Serial.printf("Selected %s", currentDirectory);
            memset(result, 0, resultSize);
            strncpy(result, currentDirectory, resultSize);
            return true;
          }
        }
        if (millis() - selectedTime > timePerChar &&
            strlen(selectedPath) > maxCharPerRow - 3) {
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
                strlen(selectedPath) + 3) {
              offsetPauseTicks = startEndPauseTicks;
              resetAfterPause = true;
            }
            break;
          }
        }
        if (millis() - lastTitleShiftTime > timePerChar &&
            strlen(currentDirectory) > maxCharInTitle) {
          lastTitleShiftTime = millis();
          if (titleOffsetPauseTicks > 0) {
            titleOffsetPauseTicks--;
          } else if (titleResetAfterPause) {
            titleResetAfterPause = false;
            titleSelectedCharOffset = 0;
            titleOffsetPauseTicks = startEndPauseTicks;
            break;
          } else {
            titleSelectedCharOffset++;
            if (titleSelectedCharOffset + maxCharInTitle >=
                strlen(currentDirectory) + 1) {
              titleOffsetPauseTicks = startEndPauseTicks;
              titleResetAfterPause = true;
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
}

bool ESP32CameraGUI::changeRTCTime() {
  const uint8_t charWidth = 6;
  const uint8_t charHeight = 8;
  const uint16_t boxColor = TFT_WHITE;
  const uint16_t textColor = TFT_BLACK;
  const uint8_t topPadding = 8;
  const uint8_t rightPadding = 8;
  const uint8_t bottomPadding = 16;
  const uint8_t leftPadding = 8;

  const uint16_t boxWidth = this->tft->width() - leftPadding - rightPadding;
  const uint16_t boxHeight = this->tft->height() - topPadding - bottomPadding;
  const uint16_t boxX = leftPadding;
  const uint16_t boxY = topPadding;

  const uint8_t maxEntryPerPage = 10;
  const uint16_t timePerChar = 200;
  const uint8_t startEndPauseTicks = 4;
  const uint16_t holdToAccelTime = 500;
  const uint16_t moveThrottleTime = 50;
  const uint16_t maxActionWaitTime = 1000;

  uint32_t selectedTime = millis();
  uint32_t lastPressTime = millis();
  uint32_t pressTime = 0;
  uint32_t lastMovedTime = millis();

  const uint8_t fontX = boxX + charWidth / 2;
  uint8_t fontY = boxY + charHeight;

  this->tft->fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1,
                           boxColor);
  this->tft->drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1,
                           textColor);

  this->tft->setTextColor(textColor, boxColor);
  this->tft->setCursor(fontX, fontY);
  this->tft->print(" Set time");

  fontY += charHeight * 1.5;

  const uint8_t scrollBarX = boxX + boxWidth - charWidth;
  const uint8_t scrollBarY = fontY;
  const uint8_t scrollBarWidth = charWidth / 2;
  const uint8_t scrollBarHeight = maxEntryPerPage * charHeight;

  DateTime beforeModification = this->rtc->now();
  uint32_t startTimeInSelector = millis();

  // 0 = year, 1 = month, 2 = day, 3 = hour, 4 = minute, 5 = second
  uint8_t selectedSpan = 0;
  bool isEditing = false;

  while (true) {
    DateTime now = this->rtc->now();

    const uint8_t numberCount = 6;
    const uint16_t numbers[numberCount] = {now.year(),   now.month(),
                                           now.day(),    now.hour(),
                                           now.minute(), now.second()};
    const char* numberFormat[numberCount] = {"%d", "%d",   "%d",
                                             "%d", "%.2d", "%.2d"};
    const char* numberFormatTrail[numberCount] = {"/", "/", " ",
                                                  ":", ":", "  "};

    uint8_t currentCharX = 0;

    this->tft->setCursor(fontX + charWidth, fontY + charHeight);

    for (uint8_t i = 0; i < numberCount; i++) {
      if (selectedSpan == i && isEditing) {
        this->tft->setTextColor(boxColor, textColor);
      } else {
        this->tft->setTextColor(textColor, boxColor);
      }
      const size_t bufSize = 8;
      char buf[bufSize];
      memset(buf, 0, bufSize);
      snprintf(buf, bufSize, numberFormat[i], numbers[i]);
      this->tft->print(buf);
      this->tft->drawFastHLine(
        fontX + charWidth + currentCharX * charWidth,
        fontY + charHeight * 2 + 1, charWidth * strlen(buf),
        selectedSpan == i && !isEditing ? textColor : boxColor);
      currentCharX += strlen(buf);
      this->tft->setTextColor(textColor, boxColor);
      this->tft->print(numberFormatTrail[i]);
      currentCharX += strlen(numberFormatTrail[i]);
    }

    this->tft->setCursor(fontX + charWidth, fontY + charHeight * 3);
    if (selectedSpan == 6) {
      this->tft->setTextColor(boxColor, textColor);
    } else {
      this->tft->setTextColor(textColor, boxColor);
    }
    this->tft->print("Save and exit");

    this->tft->setCursor(fontX + charWidth, fontY + charHeight * 4);
    if (selectedSpan == 7) {
      this->tft->setTextColor(boxColor, textColor);
    } else {
      this->tft->setTextColor(textColor, boxColor);
    }
    this->tft->print("Cancel and exit");

    this->drawBottomToolbar();

    while (millis() - lastMovedTime < moveThrottleTime) {
      delay(1);
    }

    uint32_t startWait = millis();

    while (true) {
      this->drawBottomToolbar();

      if (!this->upButton->read() || !this->downButton->read()) {
        if (lastPressTime > 0) {
          pressTime += millis() - lastPressTime;
        }
        lastPressTime = millis();
      } else {
        pressTime = 0;
        lastPressTime = 0;
      }
      if (this->upButton->pressed() ||
          (!this->upButton->read() && pressTime > holdToAccelTime)) {
        if (isEditing) {
          DateTime now = this->rtc->now();
          switch (selectedSpan) {
            default: {
              break;
            }
            case 0: {  // year
              DateTime target =
                DateTime(now.year() + 1, now.month(), now.day(), now.hour(),
                         now.minute(), now.second());
              if (!target.isValid()) {
                target = target + TimeSpan(1, 0, 0, 0);
              }
              this->rtc->adjust(target);
              break;
            }
            case 1: {  // month
              DateTime target =
                DateTime(now.year(), now.month() + 1, now.day(), now.hour(),
                         now.minute(), now.second());
              if (!target.isValid()) {
                target = target + TimeSpan(1, 0, 0, 0);
              }
              this->rtc->adjust(target);
              break;
            }
            case 2: {  // day
              this->rtc->adjust(now + TimeSpan(1, 0, 0, 0));
              break;
            }
            case 3: {  // hour
              this->rtc->adjust(now + TimeSpan(0, 1, 0, 0));
              break;
            }
            case 4: {  // minute
              this->rtc->adjust(now + TimeSpan(0, 0, 1, 0));
              break;
            }
            case 5: {  // second
              this->rtc->adjust(now + TimeSpan(0, 0, 0, 1));
              break;
            }
          }
        } else {
          if (selectedSpan == 0) {
            selectedSpan = 7;
          } else {
            selectedSpan--;
          }
        }
        break;
      }
      if (this->downButton->pressed() ||
          (!this->downButton->read() && pressTime > holdToAccelTime)) {
        if (isEditing) {
          DateTime now = this->rtc->now();
          switch (selectedSpan) {
            default: {
              break;
            }
            case 0: {  // year
              DateTime target =
                DateTime(now.year() - 1, now.month(), now.day(), now.hour(),
                         now.minute(), now.second());
              if (!target.isValid()) {
                target = target - TimeSpan(1, 0, 0, 0);
              }
              this->rtc->adjust(target);
              break;
            }
            case 1: {  // month
              DateTime target =
                DateTime(now.year(), now.month() - 1, now.day(), now.hour(),
                         now.minute(), now.second());
              if (!target.isValid()) {
                target = target - TimeSpan(1, 0, 0, 0);
              }
              this->rtc->adjust(target);
              break;
            }
            case 2: {  // day
              this->rtc->adjust(now - TimeSpan(1, 0, 0, 0));
              break;
            }
            case 3: {  // hour
              this->rtc->adjust(now - TimeSpan(0, 1, 0, 0));
              break;
            }
            case 4: {  // minute
              this->rtc->adjust(now - TimeSpan(0, 0, 1, 0));
              break;
            }
            case 5: {  // second
              this->rtc->adjust(now - TimeSpan(0, 0, 0, 1));
              break;
            }
          }
        } else {
          if (selectedSpan == 7) {
            selectedSpan = 0;
          } else {
            selectedSpan++;
          }
        }
        break;
      }
      if (this->selectButton->pressed()) {
        if (selectedSpan <= 5) {
          isEditing = !isEditing;
          break;
        } else if (selectedSpan == 6) {
          return true;
        } else {
          this->rtc->adjust(beforeModification +
                            TimeSpan(startTimeInSelector / 1000));
          return false;
        }
      }
      if (millis() - startWait > maxActionWaitTime) {
        break;
      }
    }

    lastMovedTime = millis();
  }
}

void ESP32CameraGUI::drawBottomToolbar(bool forceDraw) {
  if (millis() - this->lastBottomToolbarDraw > BOTTOM_TOOLBAR_DRAW_THROTTLE ||
      forceDraw || needToRedrawBottom) {
    this->lastBottomToolbarDraw = millis();
    this->needToRedrawBottom = false;

    const uint8_t charWidth = 6;
    const uint8_t charHeight = 8;

    const uint16_t textX = 0;
    const uint16_t textY = this->tft->height() - charHeight;

    this->tft->setCursor(textX, textY);
    this->tft->setTextColor(TFT_WHITE, TFT_BLACK);

    if (this->hasCustomBottomText) {
      this->tft->print(this->customBottomText);
      this->tft->setTextWrap(false);
      for (uint8_t i = strlen(this->customBottomText);
           i < ESP32CameraGUI::maxBottomTextSize + 1; i++) {
        this->tft->print(" ");
      }
      this->tft->setTextWrap(true);

      if (millis() > this->customBottomTextExpire) {
        this->hasCustomBottomText = false;
        this->needToRedrawBottom = true;
      }
    } else {
      DateTime now = this->rtc->now();

      this->tft->printf("%d/%d/%d %d:%.2d ", now.year(), now.month(), now.day(),
                        now.hour(), now.minute());
      this->tft->setTextWrap(false);
      for (uint8_t i = 10; i < ESP32CameraGUI::maxBottomTextSize + 1; i++) {
        this->tft->print(" ");
      }
      this->tft->setTextWrap(true);

      const uint8_t bufSize = 16;
      char buf[bufSize];
      memset(buf, 0, bufSize);

      snprintf(buf, bufSize, "%d%%", this->getBattPercent());
      this->tft->setCursor(this->tft->width() - charWidth * strlen(buf),
                           this->tft->height() - charHeight);
      this->tft->print(buf);
    }
  }
}

void ESP32CameraGUI::setBottomText(const char* text, uint32_t expireTime) {
  strncpy(this->customBottomText, text, ESP32CameraGUI::maxBottomTextSize);
  this->needToRedrawBottom = true;
  this->hasCustomBottomText = true;
  this->customBottomTextExpire = millis() + expireTime;
}

void ESP32CameraGUI::setBottomText(char* text, uint32_t expireTime) {
  strncpy(this->customBottomText, text, ESP32CameraGUI::maxBottomTextSize);
  this->needToRedrawBottom = true;
  this->hasCustomBottomText = true;
  this->customBottomTextExpire = millis() + expireTime;
}
