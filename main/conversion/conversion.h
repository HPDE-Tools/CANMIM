#ifndef __HPDE_TOOLS_CONVERSION_H__
#define __HPDE_TOOLS_CONVERSION_H__

#include<cinttypes>

namespace hpde_tools{

class ADC_Conversion {

public:
    ADC_Conversion() {}
    ADC_Conversion(const ADC_Conversion &)  = delete;
    ADC_Conversion &operator=(const ADC_Conversion &) = delete;
    virtual void Convert(uint16_t *converted, uint16_t huv) = 0;
};

class NO_Conversion : public ADC_Conversion {

public:
    void Convert(uint16_t *converted, uint16_t huv) override {
        *converted = 0;
    }

};

class PressureSensor_Conversion: public ADC_Conversion {

private:
    uint16_t max_pressure_psi;

public:
    PressureSensor_Conversion(uint16_t pressure) : max_pressure_psi(pressure) {}
    void Convert(uint16_t *converted, uint16_t huv) override {
        uint32_t uv_100_cp = huv;
        if (uv_100_cp < 5000)
            uv_100_cp = 5000;
        uint32_t ret_pressure = this->max_pressure_psi * (uv_100_cp - 5000);
        if (ret_pressure % 40000 > 40000 / 2)
            ret_pressure = ret_pressure / 40000 + 1;
        else
            ret_pressure = ret_pressure / 40000;
        *converted = ret_pressure;
    }
};

class PT385TempSensor_Conversion: public ADC_Conversion {

private:
    float divider_resistor;

public:
    PT385TempSensor_Conversion(float resistor): divider_resistor(resistor) {}
    void Convert(uint16_t * converted, uint16_t huv) override;
};

class EthanolSensor_Conversion: public ADC_Conversion {

public:
    void Convert(uint16_t * converted, uint16_t huv) override {
        if (huv > 50000)
            huv = 5000;
        if (huv < 5000)
            huv = 5000;
        *converted = (huv - 5000)/400;
    }
};
}
#endif