#ifndef PORT_H
#define PORT_H

enum port_pin_dir {
  /** The pin's input buffer should be enabled, so that the pin state can
   *  be read */
  PORT_PIN_DIR_INPUT               = INPUT,
  /** The pin's output buffer should be enabled, so that the pin state can
   *  be set */
  PORT_PIN_DIR_OUTPUT              = OUTPUT,

};

/**
 *  \brief Port pin input pull configuration enum.
 *
 *  Enum for the possible pin pull settings of the port pin configuration
 *  structure, to indicate the type of logic level pull the pin should use.
 */
enum port_pin_pull {
  /** No logical pull should be applied to the pin */
  PORT_PIN_PULL_NONE = INPUT_FLOATING,
  /** Pin should be pulled up when idle */
  PORT_PIN_PULL_UP   = INPUT_PULLUP,
  /** Pin should be pulled down when idle */
  PORT_PIN_PULL_DOWN = INPUT_PULLDOWN,
};


struct port_config {
    /** Port buffer input/output direction */
  enum port_pin_dir  direction;

  /** Port pull-up/pull-down for input pins */
  enum port_pin_pull input_pull;
};


void port_get_config_defaults(struct port_config *const config);
void port_pin_set_config(const uint8_t gpio_pin,const struct port_config *const config);
void port_pin_set_output_level(const uint8_t gpio_pin, const bool level);
bool port_pin_get_input_level(const uint8_t gpio_pin);

#endif
