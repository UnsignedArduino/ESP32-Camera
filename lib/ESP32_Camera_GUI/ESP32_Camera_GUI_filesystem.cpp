#include <Arduino.h>
#include "ESP32_Camera_GUI.h"

bool ESP32CameraGUI::fileExplorer(const char* startDirectory, char* result,
                                  size_t resultSize, int32_t startingFileIndex,
                                  int32_t startingOffset, char* endingDirectory,
                                  int32_t endingDirectorySize,
                                  int32_t* endingFileIndex,
                                  int32_t* endingOffset) {
  const char* fileExplorerOptionsTitle = "File options";
  const uint8_t fileExplorerOptionsCount = 2;
  const char* fileExplorerOptions[fileExplorerOptionsCount] = {"Cancel",
                                                               "Delete"};

  const char* fileExplorerDeleteOptionsTitle = "Delete file?";
  const uint8_t fileExplorerDeleteOptionsCount = 2;
  const char* fileExplorerDeleteOptions[fileExplorerDeleteOptionsCount] = {
    "No", "Yes"};

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

  int32_t offset = startingOffset;
  int32_t selected = startingFileIndex;

  int32_t lastSelected = -1;
  int32_t lastOffset = -1;

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

    Serial.printf("Last selected index is %d, last offset is %d\n",
                  lastSelected, lastOffset);
    Serial.printf("Selected index is %d, offset is %d\n", selected, offset);
    if (lastSelected != -1) {
      selected = lastSelected;
      lastSelected = -1;
    }
    if (lastOffset != -1) {
      offset = lastOffset;
      lastOffset = -1;
    }
    Serial.printf("Last selected index is %d, last offset is %d\n",
                  lastSelected, lastOffset);
    Serial.printf("Selected index is %d, offset is %d\n", selected, offset);
    selected =
      constrain(selected, 0, (int32_t)min(fileCount, (uint32_t)0x7FFFFFFF));
    if (selected >= offset + maxEntryPerPage) {
      offset += selected - (offset + maxEntryPerPage) + 1;
    } else if (selected < offset) {
      offset = selected;
    }
    Serial.printf("Selected index is %d, offset is %d\n", selected, offset);

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

      this->drawBottomToolbar(true);

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
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE - 1);
            } else {
              strncat(currentDirectory, "/", MAX_PATH_SIZE - 1);
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE - 1);
            }
            if (currentDirectory[strlen(currentDirectory) - 1] == '/') {
              currentDirectory[strlen(currentDirectory) - 1] = '\0';
            }
            Serial.printf("chdir to %s (%s)\n", selectedPath, currentDirectory);
            needToCompleteRedraw = true;
            break;
          } else {
            if (endingDirectory != NULL) {
              strncpy(endingDirectory, currentDirectory, endingDirectorySize);
            }
            if (endingFileIndex != NULL) {
              *endingFileIndex = selected;
            }
            if (endingOffset != NULL) {
              *endingOffset = offset;
            }
            Serial.printf("Current directory is %s\nSelected index is %lu\n",
                          currentDirectory, selected);
            if (currentDirectory[strlen(currentDirectory) - 1] == '/') {
              currentDirectory[strlen(currentDirectory) - 1] = '\0';
            }
            if (selectedPath[0] == '/') {
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE - 1);
            } else {
              strncat(currentDirectory, "/", MAX_PATH_SIZE - 1);
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE - 1);
            }
            Serial.printf("Selected %s\n", currentDirectory);
            memset(result, 0, resultSize);
            strncpy(result, currentDirectory, resultSize);
            return true;
          }
        }
        if (this->shutterButton->pressed()) {
          bool exitFileExplorerOptionsMenu = false;
          const size_t tempNotifSize = 32;
          char tempNotif[tempNotifSize];
          memset(tempNotif, 0, tempNotifSize);
          lastOffset = offset;
          lastSelected = selected;
          while (!exitFileExplorerOptionsMenu) {
            switch (this->menu(fileExplorerOptionsTitle, fileExplorerOptions,
                               fileExplorerOptionsCount)) {
              default:
              case 0: {
                exitFileExplorerOptionsMenu = true;
                break;
              }
              case 1: {
                if (this->menu(fileExplorerDeleteOptionsTitle,
                               fileExplorerDeleteOptions,
                               fileExplorerDeleteOptionsCount) == 1) {
                  const size_t tempSize = MAX_PATH_SIZE;
                  char temp[tempSize];
                  memset(temp, 0, tempSize);
                  strncpy(temp, currentDirectory, tempSize);
                  if (temp[strlen(temp) - 1] == '/') {
                    temp[strlen(temp) - 1] = '\0';
                  }
                  if (selectedPath[0] == '/') {
                    strncat(temp, selectedPath, MAX_PATH_SIZE - 1);
                  } else {
                    strncat(temp, "/", MAX_PATH_SIZE - 1);
                    strncat(temp, selectedPath, MAX_PATH_SIZE - 1);
                  }
                  Serial.printf("Deleting file %s\n", temp);
                  const bool result = this->sd->remove(temp);
                  if (result) {
                    snprintf(tempNotif, tempNotifSize, "Deleted file %s",
                             selectedPath);
                    Serial.printf("Deleted file %s\n", temp);
                  } else {
                    snprintf(tempNotif, tempNotifSize,
                             "Failed to delete file %s", selectedPath);
                    Serial.printf("Failed to delete file %s\n", temp);
                  }
                  Serial.printf("Selected index was %d, offset was %d\n",
                                selected, offset);
                  this->getFileCount(currentDirectory, fileCount);
                  if (selected > fileExplorerOptionsCount || fileCount == 0) {
                    selected--;
                  }
                  if (selected < offset) {
                    offset = selected;
                  }
                  lastSelected = selected;
                  lastOffset = offset;
                  Serial.printf("Selected index is %d, offset is %d\n",
                                selected, offset);
                  exitFileExplorerOptionsMenu = true;
                } else {
                  Serial.printf("Canceled deleting file\n");
                  snprintf(tempNotif, tempNotifSize, "Canceled file deletion.",
                           selectedPath);
                }
                this->setBottomText(tempNotif, 3000);
                break;
              }
            }
          }
          needToCompleteRedraw = true;
          break;
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

bool ESP32CameraGUI::getFileCount(const char* start, uint32_t& result) {
  FsFile dir = this->sd->open(start, O_RDONLY);
  if (!dir) {
    return false;
  }

  FsFile file;
  result = 0;

  const size_t MAX_PATH_SIZE = 255;
  char path[MAX_PATH_SIZE];

  dir.rewindDirectory();
  while (file.openNext(&dir, O_RDONLY)) {
    file.getName(path, MAX_PATH_SIZE);
    if (!file.isHidden() && strlen(path) > 0) {
      result++;
    }
    file.close();
  }

  Serial.printf("File count in %s is %lu\n", start, result);

  if (dir.getError()) {
    return false;
  } else {
    dir.close();
    return true;
  }
}

bool ESP32CameraGUI::getFileNameFromIndex(const char* start, uint32_t index,
                                          char* result, size_t resultSize) {
  FsFile dir = this->sd->open(start, O_RDONLY);
  if (!dir) {
    return false;
  }

  FsFile file;
  dir.rewindDirectory();
  for (uint32_t i = 0; i <= index;) {
    if (!file.openNext(&dir, O_RDONLY)) {
      return false;
    }
    file.getName(result, resultSize);
    if (!file.isHidden() && strlen(result) > 0) {
      i++;
    }
  }

  file.getName(result, resultSize);

  if (file.isDir()) {
    strncat(result, "/", resultSize);
  }

  file.close();
  dir.close();

  return true;
}
