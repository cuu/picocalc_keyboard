#include "port.h"

void port_get_config_defaults(struct port_config *const config)
{

  /* Default configuration values */
  config->direction  = PORT_PIN_DIR_INPUT;
  config->input_pull = PORT_PIN_PULL_UP;

}

void port_pin_set_config(const uint8_t gpio_pin,const struct port_config *const config)
{
  if(config->direction == PORT_PIN_DIR_OUTPUT){
    pinMode(gpio_pin,OUTPUT);
    return;
  }

  if(config->direction == PORT_PIN_DIR_INPUT){

    if(config->input_pull != PORT_PIN_PULL_NONE){
      pinMode(gpio_pin,config->input_pull);
    }else{
      pinMode(gpio_pin,config->direction);
    }
  }
}

void port_pin_set_output_level(const uint8_t gpio_pin, const bool level)
{
  digitalWrite(gpio_pin,level);
}


bool port_pin_get_input_level(const uint8_t gpio_pin){

  return digitalRead(gpio_pin);
  
}
