
#include "backlight.h"

#define KBD_BACKLIGHT_SEG 4
uint8_t kbd_backlight_segs[KBD_BACKLIGHT_SEG] = {20,40,80,110};
uint8_t kbd_backlight_offset = 0;

void lcd_backlight_update_reg() {

  uint8_t val;

  val = reg_get_value(REG_ID_BKL);
  val = val & 0xff;
  
  analogWriteFrequency(10000); 
  analogWrite(PA8, val);

}

void lcd_backlight_update(int v) {
  int val;

  val = reg_get_value(REG_ID_BKL);
  val += v;
  if(val < 0) val = 0;
  if(val > 0xff) val = 0xff;
 
  analogWriteFrequency(10000); 
  analogWrite(PA8, val);  
  reg_set_value(REG_ID_BKL,val);
}

void kbd_backlight_update(int v){
  int val;

  val = reg_get_value(REG_ID_BK2);
  val += v;
  if(val < 20 ) val = 0;
  if(val > 0xff) val = 0;
 
  analogWriteFrequency(10000); 
  analogWrite(PC8, val);  
  reg_set_value(REG_ID_BK2,val);
}

void kbd_backlight_update_offset(){
  int val;
  kbd_backlight_offset++;
  kbd_backlight_offset =  kbd_backlight_offset % KBD_BACKLIGHT_SEG;
  
  val = reg_get_value(REG_ID_BK2);
  val += kbd_backlight_segs[kbd_backlight_offset];
  if(val < 20 ) val = 0;
  if(val > 0xff) val = 0;
 
  analogWriteFrequency(10000); 
  analogWrite(PC8, val);  
  reg_set_value(REG_ID_BK2,val);
}
