#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "ArduCamera.h"
#include <ArduCAM.h>

bool ArduCamera::begin() {
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
  this->camera->OV2640_set_JPEG_size(OV2640_160x120);
  this->camera->OV2640_set_Light_Mode(Auto);
  this->camera->OV2640_set_Color_Saturation(Saturation0);
  this->camera->OV2640_set_Brightness(Brightness0);
  this->camera->OV2640_set_Contrast(Contrast0);
  this->camera->OV2640_set_Special_effects(Normal);

  this->camera->clear_fifo_flag();

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
