#include <Arduino.h>
#include "ESP32_Camera_GUI.h"

bool ESP32CameraGUI::getFileCount(char* start, uint32_t& result) {
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
  if (dir.getError()) {
    return false;
  } else {
    dir.close();
    return true;
  }
}

bool ESP32CameraGUI::getFileNameFromIndex(char* start, uint32_t index,
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
