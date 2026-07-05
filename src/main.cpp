/*
  RadioLib LoRaWAN ABP Example

  ABP = Activation by Personalisation, an alternative
  to OTAA (Over the Air Activation). OTAA is preferable.

  This example will send uplink packets to a LoRaWAN network. 
  Before you start, you will have to register your device at 
  https://www.thethingsnetwork.org/
  After your device is registered, you can run this example.
  The device will join the network and start uploading data.

  LoRaWAN v1.0.4/v1.1 requires the use of persistent storage.
  As this example does not use persistent storage, running this 
  examples REQUIRES you to check "Resets frame counters"
  on your LoRaWAN dashboard. Refer to the notes or the 
  network's documentation on how to do this.
  To comply with LoRaWAN's persistent storage, refer to
  https://github.com/radiolib-org/radiolib-persistence

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN

*/

/*
  RadioLib LoRaWAN ABP - Teste de Eficiência Automatizado
  - Canal Fixo: 916.8 MHz (via modificação na lib)
  - Largura de Banda: 125 kHz
  - Varredura: SF7->SF9->SF12 e 4-20 dBm
  - Correção: Lógica de contador e reset corrigida.
*/

#include "configABP.h"
#include <arduinoFFT.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <U8g2lib.h>
#include <Adafruit_INA219.h>

#define I2C_SDA 21
#define I2C_SCL 22

Adafruit_INA219 ina219;
uint32_t total_sec = 0;
float total_mA = 0.0;

#define PERIOD_MS (10)


TickType_t xLastWakeTime;
const TickType_t xFrequency = pdMS_TO_TICKS(PERIOD_MS);
BaseType_t xWasDelayed;

void task_monitor_energia() {
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();

  loadvoltage = busvoltage + (shuntvoltage / 1000);
  power_mW = loadvoltage * current_mA;


  total_mA += current_mA;
  total_sec += 1;
  float total_mAH = total_mA / 3600.0;
  (void)total_mAH;
  serial.printf("\n----------------------------\nV: %.3f , A: %.3f, P: %.3f\n----------------------------\n", loadvoltage, current_mA, power_mW);
}

void setup() { 
  SetupBoard();
  SerialInit();
  RadioBeginSPI();
  delay(5000);

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!ina219.begin(&Wire)) {
    while(1) {
      serial.println("ERROR: INA219 Initialise");
      delay(1000);
    }
  }
  serial.println("Initialise the INA219");
  
  // Otimização: Configurar para 32V / 1A caso o consumo seja baixo,
  // ou manter o padrão de 32V / 2A.
  ina219.setCalibration_32V_1A();

  xLastWakeTime = xTaskGetTickCount();
}

void loop() { 

  task_monitor_energia();

  xWasDelayed = xTaskDelayUntil(&xLastWakeTime, xFrequency);

  if (xWasDelayed == pdFALSE) {
    serial.println("Tempo de computação maior que o período");
  }
}