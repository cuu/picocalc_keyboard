#include "reg.h"

static uint8_t regs[REG_ID_LAST];

uint8_t reg_get_value(enum reg_id reg)
{
  return regs[reg];
}

void reg_set_value(enum reg_id reg, uint8_t value)
{
  regs[reg] = value;
}

bool reg_is_bit_set(enum reg_id reg, uint8_t bit)
{
  return regs[reg] & bit;
}

void reg_set_bit(enum reg_id reg, uint8_t bit)
{
  regs[reg] |= bit;
}

/*
 * | Bit    | Name             | Description                                                        |
| ------ |:----------------:| ------------------------------------------------------------------:|
| 7      | CFG_USE_MODS     | Should Alt, Sym and the Shift keys modify the keys being reported. |
| 6      | CFG_REPORT_MODS  | Should Alt, Sym and the Shift keys be reported as well.            |
| 5      | CFG_PANIC_INT    | Currently not implemented.                                         |
| 4      | CFG_KEY_INT      | Should an interrupt be generated when a key is pressed.            |
| 3      | CFG_NUMLOCK_INT  | Should an interrupt be generated when Num Lock is toggled.         |
| 2      | CFG_CAPSLOCK_INT | Should an interrupt be generated when Caps Lock is toggled.        |
| 1      | CFG_OVERFLOW_INT | Should an interrupt be generated when a FIFO overflow happens.     |
| 0      | CFG_OVERFLOW_ON  | When a FIFO overflow happens, should the new entry still be pushed, overwriting the oldest one. If 0 then new entry is lost. |
 */
void reg_init(void)
{
  regs[REG_ID_CFG] = CFG_OVERFLOW_INT | CFG_KEY_INT | CFG_USE_MODS;
  regs[REG_ID_DEB] = 10;
  regs[REG_ID_FRQ] = 5;
  regs[REG_ID_BKL] = 255;//100%duty
}
