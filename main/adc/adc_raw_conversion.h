#ifndef __HPDE_TOOLS_ADC_RAW_CONVERSION_H__
#define __HPDE_TOOLS_ADC_RAW_CONVERSION_H__

#include<cinttypes>

namespace hpde_tools{

void ADCConversion150OilSensor(uint16_t *pressure, uint16_t uv_100);

void ADCConversion10BarOilSensor(uint16_t *pressure, uint16_t huv);

void ADCConversionEthPercentSensor(uint16_t *percent, uint16_t huv);

void ADCConversionPT385_100Temp(uint16_t *temp, uint16_t huv);

}

#endif