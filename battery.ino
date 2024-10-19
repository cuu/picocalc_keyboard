#include "battery.h"


void indicator_led_on(){
    digitalWrite(PC13, LOW);
}

void indicator_led_off(){
    digitalWrite(PC13, HIGH);
}

void flash_one_time(int ts,int restore_status) {
    for(int i=0;i<ts;i++) {
      indicator_led_on();
      delay(400);
      indicator_led_off();
      delay(200);
    }
    digitalWrite(PC13,restore_status);
}

void show_bat_segs(){
  if(!PMU.isBatteryConnect()) return;

  int pcnt =  PMU.getBatteryPercent();
  int last_d201_status = digitalRead(PC13);
  
  if(pcnt >0 && pcnt < 33) {
    //show one time
    flash_one_time(1,last_d201_status);
  }else if(pcnt >= 33 && pcnt <66){
    //show 2 times
     flash_one_time(2,last_d201_status);   
  }else if(pcnt >=66 && pcnt <= 100){
    //show 3 times
     flash_one_time(3,last_d201_status);
  }
  
  if(PMU.isCharging()){
    start_chg();
  }

}

void low_bat(){
  if(PMU.isBatteryConnect() && !PMU.isCharging()){
    int pcnt = PMU.getBatteryPercent();
     if(pcnt <= LOW_BAT_VAL){
      //This is related to the battery charging and discharging logic. If you're not sure what you're doing, please don't modify it, as it could damage the battery.
        indicator_led_off();      
        if(pcnt <= 1) {//This is related to the battery charging and discharging logic. If you're not sure what you're doing, please don't modify it, as it could damage the battery.
          PMU.setChargingLedMode(XPOWERS_CHG_LED_BLINK_4HZ);
          if(pcnt==0){//This is related to the battery charging and discharging logic. If you're not sure what you're doing, please don't modify it, as it could damage the battery.
            PMU.shutdown();//This is related to the battery charging and discharging logic. If you're not sure what you're doing, please don't modify it, as it could damage the battery.
          }
        }else{
          PMU.setChargingLedMode(XPOWERS_CHG_LED_ON);
        }
     }else{
        indicator_led_on();
        PMU.setChargingLedMode(XPOWERS_CHG_LED_OFF);
     }
  }
}

void start_chg(){
  indicator_led_on();
  PMU.setChargingLedMode(XPOWERS_CHG_LED_BLINK_1HZ);
}

void stop_chg(){
  PMU.setChargingLedMode(XPOWERS_CHG_LED_OFF);
  low_bat();
}
