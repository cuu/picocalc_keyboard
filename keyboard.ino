

#include "port.h"
#include "keyboard.h"



enum mod
{
  MOD_NONE = 0,
  MOD_SYM,
  MOD_ALT,
  MOD_SHL,
  MOD_SHR,
  MOD_CTRL,
  MOD_LAST,
};

struct entry
{
  char chr;
  char symb;
  enum mod mod;
};

struct list_item
{   
    const struct entry *p_entry;
    uint32_t hold_start_time;
    enum key_state state;
    bool mods[MOD_LAST];
};



static const struct entry kbd_entries[][NUM_OF_COLS] =
{
  {{KEY_F5,KEY_F10},      {KEY_F4,KEY_F9},  {KEY_F3,KEY_F8},{KEY_F2,KEY_F7},   {KEY_F1,KEY_F6},    {'`','~'},{'3','#'}, {'2','@'}},
  {{KEY_BACKSPACE},       {KEY_DEL,KEY_END},{KEY_CAPS_LOCK},{KEY_TAB,KEY_HOME},{KEY_ESC,KEY_BREAK},{'4','$'},{'E'},     {'W'}},
  {{'P'},                 {'=','+'},        {'-','_'},      {'\\','|'},        {'/','?'},          {'R'},    {'S'},     {'1','!'}},
  {{KEY_ENTER,KEY_INSERT},{'8','*'},        {'7','&'},      {'6','^'},         {'5','%'},          {'F'},    {'X'},     {'Q'}},
  {{'>','.'},             {'I'},            {'U'},          {'Y'},             {'T'},              {'V'},    {';',':'}, {'A'}},
  {{'L'},                 {'K'},            {'J'},          {'H'},             {'G'},              {'C'},    {'\'','"'},{'Z'}},
  {{'O'},                 {'<',','},        {'M'},          {'N'},             {'B'},              {'D'},    {' '},     { }},
  
};

static const struct entry btn_entries[NUM_OF_BTNS] =
{
  {.mod = MOD_ALT},
  {.mod = MOD_CTRL},
  {.mod = MOD_SHL},
  {.mod = MOD_SHR},
  {'0',')'},
  {'9','('},
  {']','}'},
  {'[','{'},
  {KEY_RIGHT},
  {KEY_UP},
  {KEY_DOWN},
  {KEY_LEFT}
 
};

static struct {
    lock_callback _lock_callback;
    key_callback _key_callback;

    struct list_item list[KEY_LIST_SIZE];

    uint32_t last_process_time;

    bool mods[MOD_LAST];

    bool capslock_changed;
    bool capslock;

    bool numlock_changed;
    bool numlock;
} self;

static void transition_to(struct list_item * const p_item, const enum key_state next_state)
{
  const struct entry * const p_entry = p_item->p_entry;

  p_item->state = next_state;

  if (!self._key_callback || !p_entry)
    return;

  char chr = p_entry->chr;
  if(chr == KEY_CAPS_LOCK){
    if(self.capslock == true){
      self.capslock = false;
    }else{
      self.capslock = true;
    }
    self.capslock_changed = true;
  }
  switch (p_entry->mod) {
    case MOD_ALT:
      if (reg_is_bit_set(REG_ID_CFG, CFG_REPORT_MODS))
        chr = KEY_MOD_ALT;
      break;

    case MOD_SHL:
      if (reg_is_bit_set(REG_ID_CFG, CFG_REPORT_MODS))
        chr = KEY_MOD_SHL;
      break;

    case MOD_SHR:
      if (reg_is_bit_set(REG_ID_CFG, CFG_REPORT_MODS))
        chr = KEY_MOD_SHR;
      break;

    case MOD_SYM:
      if (reg_is_bit_set(REG_ID_CFG, CFG_REPORT_MODS))
        chr = KEY_MOD_SYM;
      break;
    case MOD_CTRL:
      if (reg_is_bit_set(REG_ID_CFG, CFG_REPORT_MODS))
        chr = KEY_MOD_CTRL;
      break;      
      
    default:
    {
      if (reg_is_bit_set(REG_ID_CFG, CFG_USE_MODS)) {
        const bool shift = (self.mods[MOD_SHL] || self.mods[MOD_SHR]) | self.capslock;
        const bool alt = self.mods[MOD_ALT] | self.numlock;
        const bool ctrl = self.mods[MOD_CTRL];//shortcuts control
        
        if (shift && (chr <'A' || chr >'Z')) {
            if(self.capslock){
              if(chr == KEY_BACKSPACE || chr == KEY_ENTER){
                //keep
              }else{
                chr = p_entry->symb;
              }
            }else{
              chr = p_entry->symb;
            }
            
        }else if(self.capslock && (chr >= 'A' && chr <= 'Z')){
           //pass
        }
        else if (!shift && (chr >= 'A' && chr <= 'Z')) {
          chr = (chr + ' ');// uppercase to lowercase for a to z
        }
      }

      break;
    }
  }

  if (chr != 0) {
    self._key_callback(chr, next_state);
  }
}

static void next_item_state(struct list_item * const p_item, const bool pressed)
{
  switch (p_item->state) {
    case KEY_STATE_IDLE:
      if (pressed) {
        if (p_item->p_entry->mod != MOD_NONE)
          self.mods[p_item->p_entry->mod] = true;

        if (!self.capslock_changed && self.mods[MOD_SHR] && self.mods[MOD_ALT]) {
          self.capslock = true;
          self.capslock_changed = true;
        }

        if (!self.numlock_changed && self.mods[MOD_SHL] && self.mods[MOD_ALT]) {
          self.numlock = true;
          self.numlock_changed = true;
        }

        if (!self.capslock_changed && (self.mods[MOD_SHL] || self.mods[MOD_SHR])) {
          self.capslock = false;
          self.capslock_changed = true;
        }

        if (!self.numlock_changed && (self.mods[MOD_SHL] || self.mods[MOD_SHR])) {
          self.numlock = false;
          self.numlock_changed = true;
        }

        if (!self.mods[MOD_ALT]) {
          self.capslock_changed = false;
          self.numlock_changed = false;
        }

        if (self._lock_callback && (self.capslock_changed || self.numlock_changed))
          self._lock_callback(self.capslock_changed, self.numlock_changed);

        transition_to(p_item, KEY_STATE_PRESSED);

        p_item->hold_start_time = time_uptime_ms();
      }
      break;

    case KEY_STATE_PRESSED:
      if ((time_uptime_ms() - p_item->hold_start_time) > KEY_HOLD_TIME) {
        transition_to(p_item, KEY_STATE_HOLD);
       } else if(!pressed) {
        transition_to(p_item, KEY_STATE_RELEASED);
      }
      break;

    case KEY_STATE_HOLD:
      if (!pressed)
        transition_to(p_item, KEY_STATE_RELEASED);
      break;
    case KEY_STATE_RELEASED:
    {
      if (p_item->p_entry->mod != MOD_NONE)
        self.mods[p_item->p_entry->mod] = false;

      p_item->p_entry = NULL;
      transition_to(p_item, KEY_STATE_IDLE);
      break;
    }
  }
}



void keyboard_process(void)
{
  struct port_config port_init;

  if ((time_uptime_ms() - self.last_process_time) <= KEY_POLL_TIME)
    return;

  port_get_config_defaults(&port_init);

  for (uint8_t c = 0; c < NUM_OF_COLS; ++c) {
    uint8_t col_value = 0;
    port_init.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(col_pins[c], &port_init);
    port_pin_set_output_level(col_pins[c], 0);

    for (uint8_t r = 0; r < NUM_OF_ROWS; ++r) {
      uint8_t pin_value = port_pin_get_input_level(row_pins[r]);
      const bool pressed = (pin_value == 0);
      uint8_t row_bit = (1<<r);
      if(pressed){
        col_value &= ~row_bit; 
      }else{
        col_value |= row_bit;
      }
      const int32_t key_idx = (int32_t)((r * NUM_OF_COLS) + c);

      int32_t list_idx = -1;
      for (int32_t i = 0; i < KEY_LIST_SIZE; ++i) {
        if (self.list[i].p_entry != &((const struct entry*)kbd_entries)[key_idx])
          continue;

        list_idx = i;
        break;
      }

      if (list_idx > -1) {
        next_item_state(&self.list[list_idx], pressed);
        continue;
      }

      if (!pressed)
        continue;

      for (uint32_t i = 0 ; i < KEY_LIST_SIZE; ++i) {
        if (self.list[i].p_entry != NULL)
          continue;

        self.list[i].p_entry = &((const struct entry*)kbd_entries)[key_idx];
        self.list[i].state = KEY_STATE_IDLE;
        next_item_state(&self.list[i], pressed);

        break;
      }
    }
    io_matrix[c] = col_value; 
    port_pin_set_output_level(col_pins[c], 1);

    port_init.direction = PORT_PIN_DIR_INPUT;
    port_init.input_pull = PORT_PIN_PULL_NONE;
    port_pin_set_config(col_pins[c], &port_init);
  }

#if NUM_OF_BTNS > 0
  for (uint8_t b = 0; b < NUM_OF_BTNS; ++b) {
    uint8_t pin_value = port_pin_get_input_level(btn_pins[b]);
    const bool pressed = (pin_value == 0);
    if( b < 8 ) {// read BTN1->BTN8
      
      if(pressed){
        io_matrix[b] &= ~(1 << 7);
      }else{
        io_matrix[b] |= ( 1 << 7);
      }
    }else{//c64 joystick arrow keys
      //B12=left,, B11=down,B10 = up,B9 = right
      uint8_t btn_bts = b-8;
      if(pressed){
        js_bits &= ~(1<<btn_bts);
      }else{
        js_bits |= (1<<btn_bts);
      }
    }
    int8_t list_idx = -1;
    for (int8_t i = 0; i < KEY_LIST_SIZE; ++i) {
      if (self.list[i].p_entry != &((const struct entry*)btn_entries)[b])
        continue;

      list_idx = i;
      break;
    }

    if (list_idx > -1) {
      next_item_state(&self.list[list_idx], pressed);
      continue;
    }

    if (!pressed)
      continue;

    for (uint8_t i = 0 ; i < KEY_LIST_SIZE; ++i) {
      if (self.list[i].p_entry != NULL)
        continue;

      self.list[i].p_entry = &((const struct entry*)btn_entries)[b];
      self.list[i].state = KEY_STATE_IDLE;
      next_item_state(&self.list[i], pressed);

      break;
    }
  }
#endif
  io_matrix[8] = 0xFF;
  self.last_process_time = time_uptime_ms();
}

void keyboard_set_key_callback(key_callback callback)
{
  self._key_callback = callback;
}

void keyboard_set_lock_callback(lock_callback callback)
{
  self._lock_callback = callback;
}

bool keyboard_get_capslock(void)
{
  return self.capslock;
}

bool keyboard_get_numlock(void)
{
  return self.numlock;
}

void keyboard_init(void)
{
  struct port_config port_init;
  port_get_config_defaults(&port_init);

  for (int i = 0; i < MOD_LAST; ++i)
    self.mods[i] = false;

  // Rows
  port_init.direction = PORT_PIN_DIR_INPUT;
  port_init.input_pull = PORT_PIN_PULL_UP;
  for (uint32_t i = 0; i < NUM_OF_ROWS; ++i)
    port_pin_set_config(row_pins[i], &port_init);

  // Cols
  port_init.direction = PORT_PIN_DIR_INPUT;
  port_init.input_pull = PORT_PIN_PULL_NONE;
  for(uint32_t i = 0; i < NUM_OF_COLS; ++i)
    port_pin_set_config(col_pins[i], &port_init);

  // Btns
#if NUM_OF_BTNS > 0
  port_init.direction = PORT_PIN_DIR_INPUT;
  port_init.input_pull = PORT_PIN_PULL_UP;
  for(uint32_t i = 0; i < NUM_OF_BTNS; ++i)
    port_pin_set_config(btn_pins[i], &port_init);
#endif
}
