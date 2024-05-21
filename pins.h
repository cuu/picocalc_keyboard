#ifndef PINS_H
#define PINS_H

#include "keyboard.h"

const uint8_t row_pins[NUM_OF_ROWS] =
{
  PA0,PA1,PA2,PA3,PA4,PA5,PA6
};

const uint8_t col_pins[NUM_OF_COLS] =
{
  PC0,PC1,PC2,PC3,PC4,PC5,PC6,PC7
};

const uint8_t btn_pins[NUM_OF_BTNS] =
{
  PB0,PB1,PB2,PB3,PB4,PB5,PB6,PB7,PB12,PB13,PB14,PB15
};

#endif
