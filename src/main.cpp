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

#define PERIOD_MS (1000 * uplinkIntervalSeconds)

// Variáveis Globais de Controle
uint32_t counter = 0;
uint packet_counter = 0;

// Lista de potências (em dBm) para o teste dinâmico
static const int8_t testPowers[] = {4, 6, 8, 10, 12, 14, 16, 18, 20};
static int powerIndex = 0;

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

/**
 * @brief Gerencia a lógica do teste de varredura de SF e Potência.
 * * Esta função é chamada a cada ciclo do loop principal.
 * 1. Define o Spreading Factor (SF) com base no contador global 'packet_counter'.
 * 2. Ao final de 30 pacotes (packet_counter >= 30), avança para o próximo nível de potência.
 * 3. Aplica a configuração de potência (setTxPower) para o ciclo atual.
 * @return O valor da potência (em dBm) configurada para este ciclo.
 */
int8_t configureTransmissionParams(void) {

  // Configura e verifica potência atual
  int8_t current_power = testPowers[powerIndex];
  int16_t pwr_state = node.setTxPower(current_power);

  if (pwr_state == RADIOLIB_ERR_NONE) {
    serial.printf("Potencia ACEITA: %d dBm\n", current_power);
  } else {
      serial.printf("----------------------Potencia REJEITADA: %d dBm (Erro: %d)\n", current_power, pwr_state);
      delay(2000);
  }

  /**
   * @brief Gerencia a varredura intercalada de SF e o ciclo de Potência.
   * * 1. Usa packet_counter % 3 para intercalar: 0->SF7, 1->SF9, 2->SF12.
   * 2. Ao atingir 300 pacotes (100 de cada SF), avança a potência.
   */
  
  uint8_t sf_cycle = packet_counter % 3;

  if (sf_cycle == 0) {
    node.setDatarate(DR_SF7);
    serial.println("-> Modulacao atual: SF7");
  }
  else if (sf_cycle == 1) {
    node.setDatarate(DR_SF9);
    serial.println("-> Modulacao atual: SF9");
  }
  else if (sf_cycle == 2) {
    node.setDatarate(DR_SF12);
    serial.println("-> Modulacao atual: SF12");
  }

  return current_power;
}

void setup() { 
  SetupBoard();
  SerialInit();
  RadioBeginSPI();
  delay(5000);

  /*
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
  */
  
  serial.println("\n--- INICIO DO TESTE DE EFICIENCIA ---");
  serial.println("Initialise the radio");
  
  int state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  uint8_t ocpCurrent = 140; // mA
  int ocpState = radio.setCurrentLimit(ocpCurrent);
  if (ocpState != RADIOLIB_ERR_NONE) {
    serial.printf("Aviso: Falha ao ajustar limite de corrente (OCP): %d\n", ocpState);
  } else {
    serial.printf("Limite de Corrente (OCP) configurado para %dmA.\n", ocpCurrent);
  }

  int8_t phyPower = 20; // dBm
  int phy_state = radio.setOutputPower(phyPower); 
  if (phy_state != RADIOLIB_ERR_NONE) {
    serial.printf("Erro ao ativar PA_BOOST: %d\n", phy_state);
  } else {
    serial.printf("PA_BOOST Ativado (%d dBm liberado fisicamente).\n", phyPower);
  }

  serial.printf("Initialise LoRaWAN Network credentials\n\r");
  
  /* Configurando autenticação ABP no nó LoRa */
  node.beginABP(devAddr, NULL, NULL, nwkSEncKey, appSKey);
  node.activateABP();
  node.setADR(false);
  node.setDutyCycle(false);
  node.setDwellTime(false);
  
  serial.printf("DEVADDR: 0x%x\n\r", node.getDevAddr());
  serial.println(F("Ready!\n"));

  uint8_t uplinkPayload[242];
  snprintf((char*) uplinkPayload, sizeof(uplinkPayload), "%s", (char*)"ERROR:RESET");
  uint8_t size = strlen((char*) uplinkPayload);
  int state_ini = node.sendReceive(uplinkPayload, size);

  xLastWakeTime = xTaskGetTickCount();
}

void loop() { 
  serial.printf("\n--- Pacote %d (Ciclo atual) ---\n", packet_counter);

  // 1. Executa a lógica: Configura rádio e incrementa packet_counter internamente
  int8_t txPower = configureTransmissionParams();

  // 2. Prepara o payload
  uint8_t uplinkPayload[242];
  snprintf((char*) uplinkPayload, sizeof(uplinkPayload), "pkt|%3.lu|pwr|%2.d", packet_counter, (int)txPower);
  uint8_t size = strlen((char*) uplinkPayload);
  
  serial.printf("Packet: %s\n", (char*) uplinkPayload);

  int state = node.sendReceive(uplinkPayload, size);
  
  if (state == RADIOLIB_ERR_NONE) {
     serial.println(F("Envio OK (Sem downlink)"));
  } else if(state > 0) {
     serial.println(F("Envio OK (Downlink recebido)"));
  } else {
     debug(true, F("Error in sendReceive"), state, false);
  }

  // 5. Medição de Tempo no Ar
  RadioLibTime_t timeOnAir = node.getLastToA();
  serial.printf("Time on Air: %lu ms\n", timeOnAir);

  packet_counter++;

  if (packet_counter >= 450) {
    node.setDatarate(DR_SF7);
    serial.println("Set SF for SF7");

    packet_counter = 0;

    serial.println("--- CICLO DE 90 PACOTES CONCLUIDO ---");
    serial.println("--- AVANCANDO PARA PROXIMA POTENCIA ---");
    powerIndex++; 
    
    // Verifica se acabou todas as potências (Fim total do teste)
    const size_t NUM_POWERS = sizeof(testPowers) / sizeof(testPowers[0]);
    if (powerIndex >= NUM_POWERS) {
      powerIndex = 0;
      serial.println("--- REINICIANDO TESTE COMPLETO (voltando para 5 dBm) ---");
    }
    else {
      serial.printf("--- AVANCANDO PARA %d dBm ---\n", testPowers[powerIndex]);
    }
  }

  //task_monitor_energia();

  xWasDelayed = xTaskDelayUntil(&xLastWakeTime, xFrequency);

  if (xWasDelayed == pdFALSE) {
    // warnning
    serial.println("Tempo de computação maior que o período");
  }
}