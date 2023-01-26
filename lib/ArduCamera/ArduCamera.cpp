#include <Arduino.h>
#include "ArduCamera.h"

bool ArduCamera::begin(SdFs* sd) {
  if (this->began) {
    return true;
  }

  Wire.begin();

  this->hspi = new SPIClass(HSPI);
  this->hspi->begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI);
  this->hspi->setFrequency(8000000);
  pinMode(CAM_CS, OUTPUT);

  this->camera = new ArduCAM(OV2640, CAM_CS);

  if (!this->isConnected()) {
    return false;
  }

  this->camera->set_format(JPEG);
  this->camera->InitCAM();
  this->setImageSize(OV2640_160x120);
  this->setLightMode(Auto);
  this->setSaturation(Saturation0);
  this->setBrightness(Brightness0);
  this->setContrast(Contrast0);
  this->setSpecialEffect(Normal);

  this->camera->clear_fifo_flag();

  this->sd = sd;

  Serial.println("Camera initialization...ok!");

  this->began = true;
  return true;
}

bool ArduCamera::end() {
  if (!this->began) {
    return true;
  }

  Wire.end();

  this->hspi->end();
  free(this->hspi);
  this->hspi = NULL;

  free(this->camera);
  this->camera = NULL;

  this->began = false;
  return true;
}

bool ArduCamera::isConnected() {
  Serial.print("Testing HSPI...");
  this->camera->setSpiBus(this->hspi);
  this->camera->write_reg(ARDUCHIP_TEST1, 0x55);
  uint8_t temp = this->camera->read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55) {
    Serial.print("error! (0x55 != 0x");
    Serial.print(temp, HEX);
    Serial.println(")");
    return false;
  } else {
    Serial.println("ok!");
  }

  Serial.print("Checking camera presence...");

  uint8_t vid, pid;
  this->camera->wrSensorReg8_8(0xFF, 0x01);
  this->camera->rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  this->camera->rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26) && ((pid != 0x41) || (pid != 0x42))) {
    Serial.println("missing! (vid: 0x");
    Serial.print(vid, HEX);
    Serial.print(" pid: 0x");
    Serial.print(pid, HEX);
    Serial.println(")");
    return false;
  } else {
    Serial.println("ok!");
  }

  return true;
}

size_t ArduCamera::captureToMemory(uint8_t* dest, size_t destSize) {
  // Serial.println("Starting capture");

  this->camera->flush_fifo();
  this->camera->clear_fifo_flag();
  this->camera->start_capture();
  while (!this->camera->get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    ;
  }
  uint32_t len = this->camera->read_fifo_length();
  if (len >= MAX_FIFO_SIZE) {
    Serial.printf("FIFO oversized (%lu >= %lu)\n", len, MAX_FIFO_SIZE);
    return -1;
  } else if (len == 0) {
    Serial.println("FIFO size is 0");
    return -1;
  } else if (len > destSize) {
    Serial.println("FIFO size bigger than destination buffer");
    return -1;
  } else {
    // Serial.printf("FIFO size is %lu\n", len);
  }

  this->camera->CS_LOW();
  this->camera->set_fifo_burst();

  this->hspi->transfer(0x00);
  len--;

  size_t i = 0;

  while (len > 0) {
    dest[i] = this->hspi->transfer(0x00);
    i++;
    len--;
  }

  this->camera->CS_HIGH();

  return i;
}

int32_t ArduCamera::captureToDisk() {
  const size_t MAX_PATH_SIZE = 255;
  char filename[MAX_PATH_SIZE];
  memset(filename, 0, MAX_PATH_SIZE);
  FsFile file;
  const size_t bufSize = 4096;
  uint8_t buf[bufSize] = {};
  size_t bytesTransferred = 0;
  size_t i = 0;

  Serial.println("Starting capture to disk");

  this->camera->flush_fifo();
  this->camera->clear_fifo_flag();
  this->camera->start_capture();
  while (!this->camera->get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    ;
  }
  uint32_t len = this->camera->read_fifo_length();
  if (len >= MAX_FIFO_SIZE) {
    Serial.printf("FIFO oversized (%lu >= %lu)\n", len, MAX_FIFO_SIZE);
    goto cameraError;
  } else if (len == 0) {
    Serial.println("FIFO size is 0");
    goto cameraError;
  } else {
    Serial.printf("FIFO size is %lu\n", len);
  }

  this->getNextFilename(filename, MAX_PATH_SIZE);

  Serial.printf("Opening file %s\n", filename);

  file = this->sd->open(filename, O_WRONLY | O_CREAT | O_EXCL);

  if (!file) {
    Serial.println("Failed to open file!");
    return -2;
  } else {
    Serial.println("Open file success");
  }

  this->camera->CS_LOW();
  this->camera->set_fifo_burst();

  this->hspi->transfer(0x00);
  len--;

  while (len > 0) {
    buf[i] = this->hspi->transfer(0x00);
    if (i >= bufSize - 1) {
      if (file.write(buf, bufSize) != bufSize) {
        goto diskIOError;
      }
      i = 0;
    } else {
      i++;
    }
    bytesTransferred++;
    len--;
  }

  if (file.write(buf, i + 1) != i + 1) {
    goto diskIOError;
  }
  file.close();

  this->camera->CS_HIGH();

  Serial.printf("Capture to disk finished (wrote %hu bytes)\n",
                bytesTransferred);
  return bytesTransferred;

cameraError:
  file.close();
  this->camera->CS_HIGH();

  Serial.println("Capture to disk failed with camera error!");

  return CAMERA_ERROR;

diskIOError:
  file.close();
  this->camera->CS_HIGH();

  Serial.println("Capture to disk failed with disk IO error!");

  return DISK_IO_ERROR;
}

void ArduCamera::setImageSize(uint8_t size) {
  this->camera->OV2640_set_JPEG_size(size);
  this->imageSize = size;
}

void ArduCamera::setLightMode(uint8_t mode) {
  this->camera->OV2640_set_Light_Mode(mode);
  this->lightMode = mode;
}

void ArduCamera::setSaturation(uint8_t mode) {
  this->camera->OV2640_set_Color_Saturation(mode);
  this->saturation = mode;
}

void ArduCamera::setBrightness(uint8_t brightness) {
  this->camera->OV2640_set_Brightness(brightness);
  this->brightness = brightness;
}

void ArduCamera::setContrast(uint8_t contrast) {
  this->camera->OV2640_set_Contrast(contrast);
  this->contrast = contrast;
}

void ArduCamera::setSpecialEffect(uint8_t effect) {
  this->camera->OV2640_set_Special_effects(effect);
  this->specialEffect = effect;
}

uint8_t ArduCamera::getImageSize() { return this->imageSize; }

uint8_t ArduCamera::getLightMode() { return this->lightMode; }

uint8_t ArduCamera::getSaturation() { return this->saturation; }

uint8_t ArduCamera::getBrightness() { return this->brightness; }

uint8_t ArduCamera::getContrast() { return this->contrast; }

uint8_t ArduCamera::getSpecialEffect() { return this->specialEffect; }

void ArduCamera::getNextFilename(char* dest, size_t destSize) {
  if (!this->sd->exists("/images/")) {
    this->sd->mkdir("/images/");
    Serial.println("Created directory /images/");
  }

  while (true) {
    snprintf(dest, destSize, "/images/%010u.jpg", this->nextImageNumber);
    FsFile file = this->sd->open(dest, O_READ);
    if (!file) {
      break;
    } else {
      file.close();
      this->nextImageNumber++;
    }
  }
}
