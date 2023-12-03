#ifndef __HPDE_TOOLS_TASKS_H__
#define __HPDE_TOOLS_TASKS_H__

namespace hpde_tools {

void CANInTask(void *pvParameter);

void CANOutTask(void *pvParameter);

void ADCTLA2518Task(void *pvParameter);

void ADS1115ADCTask(void *pvParameter);

void BLETask(void *pvParameter);

void LEDTask(void *pvParameter);
}

#endif