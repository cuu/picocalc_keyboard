#include <Arduino.h>

#define XPOWERS_CHIP_AXP2101

#include <Wire.h>

#include "XPowersLib.h"
//---------------------------------------
#include "backlight.h"
#include "conf_app.h"
#include "fifo.h"
#include "keyboard.h"
#include "pins.h"
#include "port.h"
#include "reg.h"
#include "battery.h"

#define DEBUG_UART
TwoWire Wire2 = TwoWire(CONFIG_PMU_SDA, CONFIG_PMU_SCL);
bool pmu_flag = 0;
bool pmu_online = 0;
uint8_t pmu_status = 0;
uint8_t keycb_start = 0;

uint8_t head_phone_status=LOW;

XPowersPMU PMU;

const uint8_t i2c_sda = CONFIG_PMU_SDA;
const uint8_t i2c_scl = CONFIG_PMU_SCL;
const uint8_t pmu_irq_pin = CONFIG_PMU_IRQ;

unsigned long run_time;

void set_pmu_flag(void) { pmu_flag = true; }

HardwareSerial Serial1(PA10, PA9);

uint8_t write_buffer[10];
uint8_t write_buffer_len = 0;

uint8_t io_matrix[9];//for IO matrix,last bytye is the restore key(c64 only)
uint8_t js_bits=0xff;// c64 joystick bits

unsigned long time_uptime_ms() { return millis(); }

void lock_cb(bool caps_changed, bool num_changed) {
  bool do_int = false;

  if (caps_changed && reg_is_bit_set(REG_ID_CFG, CFG_CAPSLOCK_INT)) {
    reg_set_bit(REG_ID_INT, INT_CAPSLOCK);
    do_int = true;
  }

  if (num_changed && reg_is_bit_set(REG_ID_CFG, CFG_NUMLOCK_INT)) {
    reg_set_bit(REG_ID_INT, INT_NUMLOCK);
    do_int = true;
  }

  /* // int_pin can be a LED
  if (do_int) {
    port_pin_set_output_level(int_pin, 0);
    delay_ms(INT_DURATION_MS);
    port_pin_set_output_level(int_pin, 1);
  }*/
}

static void key_cb(char key, enum key_state state) {
  bool do_int = false;

  if(keycb_start == 0){
     fifo_flush();
     return;
  }
  
  if (reg_is_bit_set(REG_ID_CFG, CFG_KEY_INT)) {
    reg_set_bit(REG_ID_INT, INT_KEY);
    do_int = true;
  }

#ifdef DEBUG
  // Serial1.println("key: 0x%02X/%d/%c, state: %d, blk: %d\r\n", key, key, key,
  // state, reg_get_value(REG_ID_BKL));
#endif

  const struct fifo_item item = {key, state};
  if (!fifo_enqueue(item)) {
    if (reg_is_bit_set(REG_ID_CFG, CFG_OVERFLOW_INT)) {
      reg_set_bit(REG_ID_INT, INT_OVERFLOW);  // INT_OVERFLOW  The interrupt was
                                              // generated by FIFO overflow.
      do_int = true;
    }

    if (reg_is_bit_set(REG_ID_CFG, CFG_OVERFLOW_ON)) fifo_enqueue_force(item);
  }

  //Serial1.println(key);
}

void receiveEvent(int howMany) {
  uint8_t rcv_data[2];  // max size 2, protocol defined
  uint8_t rcv_idx;

  if (Wire.available() < 1) return;

  rcv_idx = 0;
  while (Wire.available())  // loop through all but the last
  {
    uint8_t c = Wire.read();  // receive byte as a character
    rcv_data[rcv_idx] = c;
    rcv_idx++;
    if (rcv_idx >= 2) {
      rcv_idx = 0;
    }
  }

  const bool is_write = (rcv_data[0] & WRITE_MASK);
  const uint8_t reg = (rcv_data[0] & ~WRITE_MASK);

  write_buffer[0] = 0;
  write_buffer[1] = 0;
  write_buffer_len = 2;
  
  switch (reg) {
    case REG_ID_FIF: {
      const struct fifo_item item = fifo_dequeue();
      write_buffer[0] = (uint8_t)item.state;
      write_buffer[1] = (uint8_t)item.key;
    } break;
    case REG_ID_BKL: {
      reg_set_value(REG_ID_BKL, rcv_data[1]);
      lcd_backlight_update_reg();
      write_buffer[0] = reg;
      write_buffer[1] = reg_get_value(REG_ID_BKL);
    } break;
    case REG_ID_BAT:{
      write_buffer[0] = reg;
      if (PMU.isBatteryConnect()) {
        write_buffer[1] = PMU.getBatteryPercent();
      }else{
        write_buffer[1] = 0x00;
      }
    }break;
    case REG_ID_KEY: {
      write_buffer[0] = fifo_count();
      write_buffer[0] |= keyboard_get_numlock()  ? KEY_NUMLOCK  : 0x00;
      write_buffer[0] |= keyboard_get_capslock() ? KEY_CAPSLOCK : 0x00;

    }break;
    case REG_ID_C64_MTX:{
      write_buffer[0] = reg;
      memcpy(write_buffer + 1, io_matrix, sizeof(io_matrix));
      write_buffer_len = 10;
    }break;
    case REG_ID_C64_JS:{
      write_buffer[0] = reg;
      write_buffer[1] = js_bits;
      write_buffer_len = 2;
    }break;
    default: {
      write_buffer[0] = 0;
      write_buffer[1] = 0;

      write_buffer_len = 2;
    } break;
  }
}

//-this is after receiveEvent-------------------------------
void requestEvent() { Wire.write(write_buffer,write_buffer_len ); }

void report_bat(){
 if (PMU.isBatteryConnect()) {
    write_buffer[0] = REG_ID_BAT;
    write_buffer[1] = PMU.getBatteryPercent();

    write_buffer_len = 2;  
    requestEvent();
  }
}

void printPMU() {
  Serial1.print("isCharging:");
  Serial1.println(PMU.isCharging() ? "YES" : "NO");
  Serial1.print("isDischarge:");
  Serial1.println(PMU.isDischarge() ? "YES" : "NO");
  Serial1.print("isStandby:");
  Serial1.println(PMU.isStandby() ? "YES" : "NO");
  Serial1.print("isVbusIn:");
  Serial1.println(PMU.isVbusIn() ? "YES" : "NO");
  Serial1.print("isVbusGood:");
  Serial1.println(PMU.isVbusGood() ? "YES" : "NO");
  Serial1.print("getChargerStatus:");
  uint8_t charge_status = PMU.getChargerStatus();
  if (charge_status == XPOWERS_AXP2101_CHG_TRI_STATE) {
    Serial1.println("tri_charge");
  } else if (charge_status == XPOWERS_AXP2101_CHG_PRE_STATE) {
    Serial1.println("pre_charge");
  } else if (charge_status == XPOWERS_AXP2101_CHG_CC_STATE) {
    Serial1.println("constant charge");
  } else if (charge_status == XPOWERS_AXP2101_CHG_CV_STATE) {
    Serial1.println("constant voltage");
  } else if (charge_status == XPOWERS_AXP2101_CHG_DONE_STATE) {
    Serial1.println("charge done");
  } else if (charge_status == XPOWERS_AXP2101_CHG_STOP_STATE) {
    Serial1.println("not chargin");
  }

  Serial1.print("getBattVoltage:");
  Serial1.print(PMU.getBattVoltage());
  Serial1.println("mV");
  Serial1.print("getVbusVoltage:");
  Serial1.print(PMU.getVbusVoltage());
  Serial1.println("mV");
  Serial1.print("getSystemVoltage:");
  Serial1.print(PMU.getSystemVoltage());
  Serial1.println("mV");

  // The battery percentage may be inaccurate at first use, the PMU will
  // automatically learn the battery curve and will automatically calibrate the
  // battery percentage after a charge and discharge cycle
  if (PMU.isBatteryConnect()) {
    Serial1.print("getBatteryPercent:");
    Serial1.print(PMU.getBatteryPercent());
    Serial1.println("%");
  }

  Serial1.println();
}

void check_pmu_int() {
  /// 40 secs check battery percent

  int pcnt;

  if (!pmu_online) return;

  if (time_uptime_ms() - run_time > 40000) {
    run_time = millis();  // reset time
    pcnt = PMU.getBatteryPercent();
    if (pcnt < 0) {  // disconnect
      pcnt = 0;
      pmu_status = 0xff;
    } else {  // battery connected
      if (PMU.isCharging()) {
        pmu_status = bitSet(pcnt, 7);
      } else {
        pmu_status = pcnt;
      }
      low_bat();
    }
  }

  if (pmu_flag) {
    pmu_flag = false;

    // Get PMU Interrupt Status Register
    uint32_t status = PMU.getIrqStatus();
    Serial1.print("STATUS => HEX:");
    Serial1.print(status, HEX);
    Serial1.print(" BIN:");
    Serial1.println(status, BIN);


    // When the set low-voltage battery percentage warning threshold is reached,
    // set the threshold through getLowBatWarnThreshold( 5% ~ 20% )
    if (PMU.isDropWarningLevel2Irq()) {
      Serial1.println("isDropWarningLevel2");
      report_bat();
    }

    // When the set low-voltage battery percentage shutdown threshold is reached
    // set the threshold through setLowBatShutdownThreshold()    
    if (PMU.isDropWarningLevel1Irq()) {
      report_bat();
      //
      PMU.shutdown();
    }
    if (PMU.isGaugeWdtTimeoutIrq()) {
      Serial1.println("isWdtTimeout");
    }
    if (PMU.isBatChargerOverTemperatureIrq()) {
      Serial1.println("isBatChargeOverTemperature");
    }
    if (PMU.isBatWorkOverTemperatureIrq()) {
      Serial1.println("isBatWorkOverTemperature");
    }
    if (PMU.isBatWorkUnderTemperatureIrq()) {
      Serial1.println("isBatWorkUnderTemperature");
    }
    if (PMU.isVbusInsertIrq()) {
      Serial1.println("isVbusInsert");
    }
    if (PMU.isVbusRemoveIrq()) {
      Serial1.println("isVbusRemove");
      stop_chg();
    }
    if (PMU.isBatInsertIrq()) {
      pcnt = PMU.getBatteryPercent();
      if (pcnt < 0) {  // disconnect
        pcnt = 0;
        pmu_status = 0xff;
      } else {
        pmu_status = pcnt;
      }

      Serial1.println("isBatInsert");
    }
    if (PMU.isBatRemoveIrq()) {
      pmu_status = 0xff;
      Serial1.println("isBatRemove");
      stop_chg();
    }

    if (PMU.isPekeyShortPressIrq()) {
      Serial1.println("isPekeyShortPress");
      // enterPmuSleep();

      Serial1.print("Read pmu data buffer .");
      uint8_t data[4] = {0};
      PMU.readDataBuffer(data, XPOWERS_AXP2101_DATA_BUFFER_SIZE);
      for (int i = 0; i < 4; ++i) {
        Serial1.print(data[i]);
        Serial1.print(",");
      }
      Serial1.println();

      printPMU();
    }

    if (PMU.isPekeyLongPressIrq()) {
      Serial1.println("isPekeyLongPress");
      //Serial1.println("write pmu data buffer .");
      //uint8_t data[4] = {1, 2, 3, 4};
      //PMU.writeDataBuffer(data, XPOWERS_AXP2101_DATA_BUFFER_SIZE);
      digitalWrite(PA13, LOW);
      digitalWrite(PA14, LOW);
      PMU.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
      PMU.shutdown();
    }

    if (PMU.isPekeyNegativeIrq()) {
      Serial1.println("isPekeyNegative");
    }
    if (PMU.isPekeyPositiveIrq()) {
      Serial1.println("isPekeyPositive");
    }

    if (PMU.isLdoOverCurrentIrq()) {
      Serial1.println("isLdoOverCurrentIrq");
    }
    if (PMU.isBatfetOverCurrentIrq()) {
      Serial1.println("isBatfetOverCurrentIrq");
    }
    if (PMU.isBatChagerDoneIrq()) {
      pcnt = PMU.getBatteryPercent();
      if (pcnt < 0) {  // disconnect
        pcnt = 0;
        pmu_status = 0xff;
      }
      pmu_status = bitClear(pcnt, 7);
      Serial1.println("isBatChagerDone");
      stop_chg();
    }
    if (PMU.isBatChagerStartIrq()) {
      pcnt = PMU.getBatteryPercent();
      if (pcnt < 0) {  // disconnect
        pcnt = 0;
        pmu_status = 0xff;
      }
      pmu_status = bitSet(pcnt, 7);
      Serial1.println("isBatChagerStart");
      if(PMU.isBatteryConnect()) {
        start_chg();
      }
    }
    if (PMU.isBatDieOverTemperatureIrq()) {
      Serial1.println("isBatDieOverTemperature");
    }
    if (PMU.isChagerOverTimeoutIrq()) {
      Serial1.println("isChagerOverTimeout");
    }
    if (PMU.isBatOverVoltageIrq()) {
      Serial1.println("isBatOverVoltage");
    }
    // Clear PMU Interrupt Status Register
    PMU.clearIrqStatus();
  }

  reg_set_value(REG_ID_BAT, (uint8_t)pmu_status);
}

/*
 * PA8 lcd_bl
 * PC8 keyboard_bl
 */
void setup() {
  pinMode(PA13, OUTPUT);  // pico enable
  digitalWrite(PA13, LOW);
  reg_init();
  
  Serial1.begin(115200);

  Wire.setSDA(PB9);
  Wire.setSCL(PB8);
  Wire.begin(SLAVE_ADDRESS);
  Wire.setClock(10000);//It is important to set to 10Khz
  Wire.onReceive(receiveEvent);  // register event
  Wire.onRequest(requestEvent);

  // no delay here
   
  bool result = PMU.begin(Wire2, AXP2101_SLAVE_ADDRESS, i2c_sda, i2c_scl);

  if (result == false) {
    Serial1.println("PMU is not online...");
  } else {
    pmu_online = 1;
    Serial1.printf("getID:0x%x\n", PMU.getChipID());
  }
   
  pinMode(PC12, INPUT);  // HP_DET

  pinMode(PC13, OUTPUT);  // indicator led

  digitalWrite(PC13, LOW);

  pinMode(PA14, OUTPUT);  // PA_EN
  digitalWrite(PA14, HIGH);
  
  int pin = PC8;

  /*
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
  // 重映射Timer3的部分映射到PC6-PC9
  AFIO->MAPR &= ~AFIO_MAPR_TIM3_REMAP;
  AFIO->MAPR |= AFIO_MAPR_TIM3_REMAP_PARTIALREMAP;
  */
  /*
  pinMode(pin,OUTPUT);

  TIM_TypeDef *Instance = (TIM_TypeDef
  *)pinmap_peripheral(digitalPinToPinName(pin), PinMap_PWM); uint32_t channel =
  STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(pin), PinMap_PWM));
  // Instantiate HardwareTimer object. Thanks to 'new' instantiation,
  HardwareTimer is not destructed when setup() function is finished.
  HardwareTimer *MyTim = new HardwareTimer(Instance);

  // Configure and start PWM
  // MyTim->setPWM(channel, pin, 5, 10, NULL, NULL); // No callback required, we
  can simplify the function call MyTim->setPWM(channel, pin, 80000, 1); //
  Hertz, dutycycle
  */
  /*
   * data records:
   * 500,10  === nightlight watch level
   */
  /*
  analogWriteFrequency(80000);
  analogWrite(pin, 10);
  */

  pin = PA8;
  analogWriteFrequency(10000);
  analogWrite(pin, 100);

  keyboard_init();
  keyboard_set_key_callback(key_cb);

  digitalWrite(PA13, HIGH);


  // It is necessary to disable the detection function of the TS pin on the
  // board without the battery temperature detection function, otherwise it will
  // cause abnormal charging
  PMU.setSysPowerDownVoltage(2750);
  PMU.disableTSPinMeasure();
  // PMU.enableTemperatureMeasure();
  PMU.enableBattDetection();
  PMU.enableVbusVoltageMeasure();
  PMU.enableBattVoltageMeasure();
  PMU.enableSystemVoltageMeasure();
  
  PMU.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);

  pinMode(pmu_irq_pin, INPUT_PULLUP);
  attachInterrupt(pmu_irq_pin, set_pmu_flag, FALLING);

  PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  PMU.clearIrqStatus();
  PMU.enableIRQ(XPOWERS_AXP2101_BAT_INSERT_IRQ |
                XPOWERS_AXP2101_BAT_REMOVE_IRQ |  // BATTERY
                XPOWERS_AXP2101_VBUS_INSERT_IRQ |
                XPOWERS_AXP2101_VBUS_REMOVE_IRQ |  // VBUS
                XPOWERS_AXP2101_PKEY_SHORT_IRQ |
                XPOWERS_AXP2101_PKEY_LONG_IRQ |  // POWER KEY
                XPOWERS_AXP2101_BAT_CHG_DONE_IRQ |
                XPOWERS_AXP2101_BAT_CHG_START_IRQ  // CHARGE
                // XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ |
                // XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
  );
    // setLowBatWarnThreshold Range:  5% ~ 20%
    // The following data is obtained from actual testing , Please see the description below for the test method.
    // 20% ~= 3.7v
    // 15% ~= 3.6v
    // 10% ~= 3.55V
    // 5%  ~= 3.5V
    // 1%  ~= 3.4V
  PMU.setLowBatWarnThreshold(5); // Set to trigger interrupt when reaching 5%

    // setLowBatShutdownThreshold Range:  0% ~ 15%
    // The following data is obtained from actual testing , Please see the description below for the test method.
    // 15% ~= 3.6v
    // 10% ~= 3.55V
    // 5%  ~= 3.5V
    // 1%  ~= 3.4V
  PMU.setLowBatShutdownThreshold(1);  // Set to trigger interrupt when reaching 1%


  
  run_time = 0;
  keycb_start = 1;
  low_bat();
  //printf("Start pico");
}

//hp headphone
void check_hp_det(){
  int v = digitalRead(PC12);
  if(v == HIGH) {
    if( head_phone_status != v ) {
      Serial1.println("HeadPhone detected");
    }
    digitalWrite(PA14,LOW);
  }else{
    digitalWrite(PA14,HIGH);
  }
  head_phone_status = v;
  
}
void loop() {
  check_pmu_int();
  keyboard_process();
  check_hp_det();
  delay(10);
}
