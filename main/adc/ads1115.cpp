/**************************************************************************/
/*! 
    This file is derived from Adafruit ADS1X15 library, modified by
    @zeroomega for ESP-IDF.
  
    Original License Text:
    @file     Adafruit_ADS1X15.cpp
    @author   K.Townsend (Adafruit Industries)

    @mainpage Adafruit ADS1X15 ADC Breakout Driver

    @section intro_sec Introduction

    This is a library for the Adafruit ADS1X15 ADC breakout boards.

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!

    @section author Author

    Written by Kevin "KTOWN" Townsend for Adafruit Industries.

    @section  HISTORY

    v1.0  - First release
    v1.1  - Added ADS1115 support - W. Earl
    v2.0  - Refactor - C. Nelson

    @section license License

    BSD license, all text here must be included in any redistribution
*/

#include "ads1115.h"

#include "esp_log.h"

/**************************************************************************/
/*!
    @brief  Sets up the HW (reads coefficients values, etc.)

    @param i2c_addr I2C address of device
    @param wire I2C bus

    @return true if successful, otherwise false
*/
/**************************************************************************/
bool ESP_ADS1X15::begin(i2c_port_t port_num, uint8_t addr) {
  this->i2c_port = port_num;
  this->i2c_addr = addr;
  return true;
}

/**************************************************************************/
/*!
    @brief  Sets the gain and input voltage range

    @param gain gain setting to use
*/
/**************************************************************************/
void ESP_ADS1X15::setGain(adsGain_t gain) { m_gain = gain; }

/**************************************************************************/
/*!
    @brief  Gets a gain and input voltage range

    @return the gain setting
*/
/**************************************************************************/
adsGain_t ESP_ADS1X15::getGain() { return m_gain; }

/**************************************************************************/
/*!
    @brief  Sets the data rate

    @param rate the data rate to use
*/
/**************************************************************************/
void ESP_ADS1X15::setDataRate(uint16_t rate) { m_dataRate = rate; }

/**************************************************************************/
/*!
    @brief  Gets the current data rate

    @return the data rate
*/
/**************************************************************************/
uint16_t ESP_ADS1X15::getDataRate() { return m_dataRate; }

ESP_ADS1X15::ERROR ESP_ADS1X15::readADC_SingleEnded_Blocking(uint8_t channel, int16_t *value) {
  if (channel > 3) {
    return ESP_ADS1X15::ERROR_FAIL;
  }

  ESP_ADS1X15::ERROR err = startADCReading(MUX_BY_CHANNEL[channel], /*continuous=*/false);
  if (err != ESP_ADS1X15::ERROR_OK)
    return err;

  // Wait for the conversion to complete
  while (!conversionComplete())
    ;

  // Read the conversion results
  return getLastConversionResults(value);
}

ESP_ADS1X15::ERROR ESP_ADS1X15::getLastConversionResults(int16_t * value) {
  // Read the conversion results
  uint16_t res;
  ESP_ADS1X15::ERROR err = readRegister(ADS1X15_REG_POINTER_CONVERT, &res);
  if (err != ESP_ADS1X15::ERROR_OK)
    return err;
  *value = (int16_t)res;
  return ESP_ADS1X15::ERROR_OK;
}

int32_t ESP_ADS1X15::computeVolts(int16_t counts) {
  int32_t fsRange;
  switch (m_gain) {
  case GAIN_TWOTHIRDS:
    fsRange = 61440;
    break;
  case GAIN_ONE:
    fsRange = 40960;
    break;
  case GAIN_TWO:
    fsRange = 20480;
    break;
  case GAIN_FOUR:
    fsRange = 10240;
    break;
  case GAIN_EIGHT:
    fsRange = 5120;
    break;
  case GAIN_SIXTEEN:
    fsRange = 2560;
    break;
  default:
    fsRange = 0.0f;
  }
  return fsRange * counts / 32768;
}


ESP_ADS1X15::ERROR ESP_ADS1X15::startADCReading(uint16_t mux, bool continuous) {
  // Start with default values
  uint16_t config =
      ADS1X15_REG_CONFIG_CQUE_1CONV |   // Set CQUE to any value other than
                                        // None so we can use it in RDY mode
      ADS1X15_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
      ADS1X15_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
      ADS1X15_REG_CONFIG_CMODE_TRAD;    // Traditional comparator (default val)

  if (continuous) {
    config |= ADS1X15_REG_CONFIG_MODE_CONTIN;
  } else {
    config |= ADS1X15_REG_CONFIG_MODE_SINGLE;
  }

  // Set PGA/voltage range
  config |= m_gain;

  // Set data rate
  config |= m_dataRate;

  // Set channels
  config |= mux;

  // Set 'start single-conversion' bit
  config |= ADS1X15_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  ESP_ADS1X15::ERROR err = writeRegister(ADS1X15_REG_POINTER_CONFIG, config);
  if (err != ESP_ADS1X15::ERROR_OK)
    return err;

  // Set ALERT/RDY to RDY mode.
  err = writeRegister(ADS1X15_REG_POINTER_HITHRESH, 0x8000);
  if (err != ESP_ADS1X15::ERROR_OK)
    return err;
  err = writeRegister(ADS1X15_REG_POINTER_LOWTHRESH, 0x0000);
  return err;
}

/**************************************************************************/
/*!
    @brief  Returns true if conversion is complete, false otherwise.

    @return True if conversion is complete, false otherwise.
*/
/**************************************************************************/
bool ESP_ADS1X15::conversionComplete() {
  uint16_t res;
  ESP_ADS1X15::ERROR err = readRegister(ADS1X15_REG_POINTER_CONFIG, &res);
  if (err != ESP_ADS1X15::ERROR_OK)
    return false;

  return (res & 0x8000) != 0;
}

/**************************************************************************/
/*!
    @brief  Writes 16-bits to the specified destination register

    @param reg register address to write to
    @param value value to write to register
*/
/**************************************************************************/
ESP_ADS1X15::ERROR ESP_ADS1X15::writeRegister(uint8_t reg, uint16_t value) {
  ESP_ADS1X15::ERROR ret;
  buffer[0] = reg;
  buffer[1] = value >> 8;
  buffer[2] = value & 0xFF;
  esp_err_t err = i2c_master_write_to_device(i2c_port, i2c_addr, buffer, 3, 1);

  switch (err) {
  case ESP_OK:
    ret = ESP_ADS1X15::ERROR_OK;
    break;
  case ESP_ERR_INVALID_STATE:
    ret = ESP_ADS1X15::ERROR_INVALID_STATE;
    break;
  case ESP_ERR_TIMEOUT:
    ret = ESP_ADS1X15::ERROR_TIME_OUT;
    break;
  default:
    ret = ESP_ADS1X15::ERROR_FAIL;
    break;
  }
  if (err != ESP_OK) {
    ESP_LOGI("I2C", "Failed to write to idc, %d ", err);
  }
  return ret;
}

/**************************************************************************/
/*!
    @brief  Read 16-bits from the specified destination register

    @param reg register address to read from

    @return 16 bit register value read
*/
/**************************************************************************/
ESP_ADS1X15::ERROR ESP_ADS1X15::readRegister(uint8_t reg, uint16_t *value) {  
  ESP_ADS1X15::ERROR ret;
  esp_err_t err = i2c_master_write_read_device(i2c_port, i2c_addr, &reg, 1, buffer, 2, 1);
  switch (err) {
  case ESP_OK:
    ret = ESP_ADS1X15::ERROR_OK;
    *value = ((buffer[0] << 8) | buffer[1]);
    break;
  case ESP_ERR_INVALID_STATE:
    ret = ESP_ADS1X15::ERROR_INVALID_STATE;
    break;
  case ESP_ERR_TIMEOUT:
    ret = ESP_ADS1X15::ERROR_TIME_OUT;
    break;
  default:
    ret = ESP_ADS1X15::ERROR_FAIL;
    break;
  }
  return ret;
}