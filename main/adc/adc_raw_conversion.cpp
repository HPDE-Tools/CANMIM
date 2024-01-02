#include "adc_raw_conversion.h"

namespace hpde_tools
{

inline static void ADCConversionOilSensor(uint32_t max_pressure, uint16_t *pressure, uint16_t uv_100)
{
    uint32_t uv_100_cp = uv_100;
    if (uv_100_cp < 5000)
        uv_100_cp = 5000;
    uint32_t ret_pressure = max_pressure * (uv_100_cp - 5000);
    if (ret_pressure % 40000 > 40000 / 2)
        ret_pressure = ret_pressure / 40000 + 1;
    else
        ret_pressure = ret_pressure / 40000;
    *pressure = ret_pressure;
}



void ADCConversion150OilSensor(uint16_t *pressure, uint16_t uv_100)
{
    ADCConversionOilSensor(1500, pressure, uv_100);
}

void ADCConversion10BarOilSensor(uint16_t *pressure, uint16_t huv) {
    ADCConversionOilSensor(1450, pressure, huv);
}

void ADCConversionEthPercentSensor(uint16_t *percent, uint16_t huv) {
    if (huv > 50000)
        huv = 5000;
    if (huv < 5000)
        huv = 5000;
    *percent = (huv - 5000)/400;
}

}