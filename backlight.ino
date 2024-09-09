
#include "backlight.h"


void lcd_backlight_update() {

  uint8_t val;

  val = reg_get_value(REG_ID_BKL);
  val = val & 0xff;
  
  analogWriteFrequency(10000); 
  analogWrite(PA8, val); 
}
void kbd_backlight_update(int level){

}
