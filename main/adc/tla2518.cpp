#include "tla2518.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include <cstring>

namespace hpde_tools
{

TLA2518::ERROR TLA2518::spiReadWrite(size_t data_len, size_t buf_len)
{
    if (buf_len < data_len)
    {
        return TLA2518::ERROR_INVALID_ARG;
    }
    spi_transaction_t tx_trans = {};
    tx_trans.length = data_len * 8;
    tx_trans.rxlength = buf_len * 8;
    if (data_len <= 4)
    {
        tx_trans.flags |= SPI_TRANS_USE_TXDATA;
        memcpy(tx_trans.tx_data, this->data_buf, data_len);
    }
    else
    {
        tx_trans.tx_buffer = this->data_buf;
    }
    if (buf_len <= 4)
    {
        tx_trans.flags |= SPI_TRANS_USE_RXDATA;
    }
    else
    {
        tx_trans.rx_buffer = this->ret_buf;
    }
    esp_err_t err = spi_device_transmit(this->device_handle, &tx_trans);
    if (err != ESP_OK)
    {
        return TLA2518::ERROR_SPI_ERROR;
    }
    // ESP_LOGI("SPI", "RXData is %hhx %hhx %hhx", (int)tx_trans.rx_data[0], (int)tx_trans.rx_data[1], (int)tx_trans.rx_data[2]);

    // ESP_LOGI("SPI", "RXData is %hhx %hhx %hhx %hhx", this->ret_buf[0], this->ret_buf[1], this->ret_buf[2], this->ret_buf[3]);

    if (buf_len <= 4)
    {
        memcpy(this->ret_buf, tx_trans.rx_data, buf_len);
    }
    return TLA2518::ERROR_OK;
}

TLA2518::ERROR TLA2518::spiReadRegister(uint8_t reg, uint8_t *data)
{
    this->data_buf[0] = RD_REG;
    this->data_buf[1] = reg,
    this->data_buf[2] = 0;
    TLA2518::ERROR err = this->spiReadWrite(3, 3);
    if (err != TLA2518::ERROR_OK)
        return err;
    memset(this->data_buf, 0, 3);
    err = this->spiReadWrite(3, 3);
    if (err != TLA2518::ERROR_OK)
        return err;
    *data = this->ret_buf[0];
    return TLA2518::ERROR_OK;
}

TLA2518::ERROR TLA2518::spiWriteRegister(uint8_t reg, uint8_t data)
{
    this->data_buf[0] = WR_REG;
    this->data_buf[1] = reg;
    this->data_buf[2] = data;
    return this->spiReadWrite(3, 3);
}

TLA2518::ERROR TLA2518::spiSetRegisterBits(uint8_t reg, uint8_t data)
{
    this->data_buf[0] = SET_BIT;
    this->data_buf[1] = reg;
    this->data_buf[2] = data;
    return this->spiReadWrite(3, 3);
}

TLA2518::ERROR TLA2518::spiClearRegisterBits(uint8_t reg, uint8_t data)
{
    this->data_buf[0] = CLEAR_BIT;
    this->data_buf[1] = reg;
    this->data_buf[2] = data;
    return this->spiReadWrite(3, 3);
}

TLA2518::ERROR TLA2518::setVRef(uint16_t huv)
{
    this->vref = huv;
    return TLA2518::ERROR_OK;
}

TLA2518::ERROR TLA2518::getSystemStatus(uint8_t *data)
{
    if (data == nullptr)
        return TLA2518::ERROR_INVALID_ARG;
    return this->spiReadRegister(TLA2518::REG_SYSTEM_STATUS, data);
}

TLA2518::ERROR TLA2518::softReset()
{
    TLA2518::ERROR err = this->spiSetRegisterBits(TLA2518::REG_GENERAL_CFG, 0x01);
    if (err != TLA2518::ERROR_OK)
        return err;
    uint8_t value = 0;
    TLA2518::ERROR ret = TLA2518::ERROR_TIMEOUT;
    for (int i = 0; i < 10; i++)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        err = this->spiReadRegister(TLA2518::REG_GENERAL_CFG, &value);
        if (err != TLA2518::ERROR_OK)
            return err;
        if ((value & 0x01) == 0)
        {
            ret = TLA2518::ERROR_OK;
            break;
        }
    }
    this->oversample = false;
    return ret;
}

TLA2518::ERROR TLA2518::offsetCalibrate()
{
    TLA2518::ERROR err = this->spiSetRegisterBits(TLA2518::REG_GENERAL_CFG, 0x02);
    if (err != TLA2518::ERROR_OK)
        return err;
    uint8_t value = 0;
    TLA2518::ERROR ret = TLA2518::ERROR_TIMEOUT;
    for (int i = 0; i < 10; i++)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        err = this->spiReadRegister(TLA2518::REG_GENERAL_CFG, &value);
        if (err != TLA2518::ERROR_OK)
            return err;
        if ((value & 0x02) == 0)
        {
            ret = TLA2518::ERROR_OK;
            break;
        }
    }
    return ret;
}

TLA2518::ERROR TLA2518::setAllAnalog()
{
    return this->spiWriteRegister(TLA2518::REG_PIN_CFG, 0);
}

TLA2518::ERROR TLA2518::clearBOR()
{
    return this->spiSetRegisterBits(TLA2518::REG_SYSTEM_STATUS, 0x01);
}

TLA2518::ERROR TLA2518::enableChID()
{
    return this->spiSetRegisterBits(TLA2518::REG_DATA_CFG, 0x01 << 4);
}

TLA2518::ERROR TLA2518::setOverSampe(uint8_t sample)
{
    TLA2518::ERROR err = this->spiWriteRegister(TLA2518::REG_OSR_CFG, sample);
    if (err == TLA2518::ERROR_OK)
    {
        if (sample == OVERSAMPLE_DISABLE)
            this->oversample = false;
        else
            this->oversample = true;
    }
    return err;
}

TLA2518::ERROR TLA2518::setSampleRate(uint8_t sampleRate)
{
    uint8_t ose_sel = sampleRate / 16;
    uint8_t divider = sampleRate % 16;
    divider |= ose_sel << 4;
    return this->spiWriteRegister(TLA2518::REG_OPMODE_CFG, divider);
}

uint8_t TLA2518::spiDataToADCRaw(uint8_t *spi_data, uint16_t *data)
{
    uint8_t ch = 0;
    *data = static_cast<uint16_t>(spi_data[0]) << 8 | spi_data[1];
    if (this->oversample)
    {
        ch = (spi_data[2] & 0xF0) >> 4;
    }
    else
    {
        *data = (*data) & 0xFFF0;
        ch = spi_data[1] & 0x0F;
    }
    return ch;
}

uint8_t TLA2518::spiDataToADCRaw(uint16_t *data)
{
    return this->spiDataToADCRaw(this->ret_buf, data);
}

TLA2518::ERROR TLA2518::readADCSingle(uint8_t ch, uint16_t *data)
{
    if (data == nullptr || ch > 7)
        return TLA2518::ERROR_INVALID_ARG;
    // Set sequence mode to manual
    TLA2518::ERROR err = this->spiWriteRegister(TLA2518::REG_SEQUENCE_CFG, 0);
    if (err != TLA2518::ERROR_OK)
        return err;
    err = this->spiWriteRegister(TLA2518::REG_CHANNEL_SEL, ch);
    if (err != TLA2518::ERROR_OK)
        return err;
    vTaskDelay(1);
    memset(this->data_buf, 0, 4);
    err = this->spiReadWrite(3, 3);
    if (err != TLA2518::ERROR_OK)
        return err;
    spiDataToADCRaw(data);
    return TLA2518::ERROR_OK;
}

/*
TLA2518::ERROR TLA2518::readADC8Ch(uint16_t *data) {
    if (data == nullptr)
        return TLA2518::ERROR_INVALID_ARG;
    // Set auto sequence ch to all
    this->spiWriteRegister(TLA2518::REG_AUTO_SEQ_CH_SEL, 0xFF);
    // Enable auto sequencing
    TLA2518::ERROR err = this->spiWriteRegister(TLA2518::REG_SEQUENCE_CFG, 0x01);
    if (err != TLA2518::ERROR_OK)
        return err;
    err = this->spiSetRegisterBits(TLA2518::REG_SEQUENCE_CFG, 0x10);
    if (err != TLA2518::ERROR_OK)
        return err;
    uint16_t value = 0;
    uint8_t ch = 0;
    memset(this->data_buf, 0, 4);
    this->spiReadWrite(3, 3);
    for (int i = 0; i < 8; i++) {
        err = this->spiReadWrite(3, 3);
        if (err != TLA2518::ERROR_OK)
            return err;
        ch = spiDataToADCRaw(&value);
        if (ch != i) {
            ESP_LOGE("ADC", "channel mismatch, i: %d, ch: %d", i, (int)ch);
        }
        data[i] = value;
    }
    // Disable auto sequencing
    err = this->spiWriteRegister(TLA2518::REG_SEQUENCE_CFG, 0x01);
    return TLA2518::ERROR_OK;
}
*/
TLA2518::ERROR TLA2518::setADCAutoSeq8ch()
{
    TLA2518::ERROR err;
    // Set auto sequence ch to all
    err = this->spiWriteRegister(TLA2518::REG_AUTO_SEQ_CH_SEL, 0xFF);
    if (err != TLA2518::ERROR_OK)
        return err;
    // Enable auto sequencing
    err = this->spiWriteRegister(TLA2518::REG_SEQUENCE_CFG, 0x01);
    if (err != TLA2518::ERROR_OK)
        return err;
    return err;
}

spi_transaction_t tx_trans[13];
inline static TLA2518::ERROR queue_trans(spi_device_t *device_handle, size_t id, uint8_t op_code, uint8_t reg, uint8_t data)
{
    tx_trans[id].length = 24;
    tx_trans[id].rxlength = 24;
    tx_trans[id].flags |= SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    tx_trans[id].tx_data[0] = op_code;
    tx_trans[id].tx_data[1] = reg;
    tx_trans[id].tx_data[2] = data;
    esp_err_t esp_err = spi_device_queue_trans(device_handle, &tx_trans[id], portMAX_DELAY);
    if (esp_err != ESP_OK)
    {
        ESP_LOGE("ADC", "failed to queue trans due to error: %d", (int)esp_err);
        return TLA2518::ERROR_SPI_ERROR;
    }
    return TLA2518::ERROR_OK;
}

TLA2518::ERROR TLA2518::readADC8Ch(uint16_t *data)
{
    if (data == nullptr)
        return TLA2518::ERROR_INVALID_ARG;
    // esp_err_t esp_err = spi_device_acquire_bus(this->device_handle, portMAX_DELAY);
    // if (esp_err != ESP_OK )
    //     return TLA2518::ERROR_INVALID_ARG;
    memset(tx_trans, 0, sizeof(spi_transaction_t) * 13);
    size_t queue_depth = 0;
    TLA2518::ERROR err;
    err = queue_trans(this->device_handle, 0, SET_BIT, TLA2518::REG_SEQUENCE_CFG, 0x10);
    if (err != TLA2518::ERROR_OK)
        goto WAIT_FOR_QUEUE;
    else
        queue_depth++;
    // Initiate read
    err = queue_trans(this->device_handle, 1, 0, 0, 0);
    if (err != TLA2518::ERROR_OK)
        goto WAIT_FOR_QUEUE;
    else
        queue_depth++;
    for (size_t i = 0; i < 8; i++)
    {
        err = queue_trans(this->device_handle, i + 2, 0, 0, 0);
        if (err != TLA2518::ERROR_OK)
            goto WAIT_FOR_QUEUE;
        else
            queue_depth++;
    }
    err = queue_trans(this->device_handle, 10, CLEAR_BIT, TLA2518::REG_SEQUENCE_CFG, 0x10);
    if (err != TLA2518::ERROR_OK)
        goto WAIT_FOR_QUEUE;
    else
        queue_depth++;

WAIT_FOR_QUEUE:
    uint8_t ch = 0;
    uint16_t value = 0;
    for (size_t i = 0; i < queue_depth; i++)
    {
        spi_transaction_t *trans;
        ESP_ERROR_CHECK(spi_device_get_trans_result(this->device_handle, &trans, portMAX_DELAY));
        if (i < 2 || i >= 10)
            continue;
        else
        {
            ch = this->spiDataToADCRaw(trans->rx_data, &value);
            if (ch != i - 2)
            {
                ESP_LOGE("ADC", "channel mismatch, i: %d, ch: %d", i - 2, (int)ch);
            }
            data[ch] = value;
        }
    }
    return err;
}

} // namespace hpde_tools
