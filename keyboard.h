#ifndef KEYBOARD_H
#define KEYBOARD_H

enum key_state
{
    KEY_STATE_IDLE = 0,
    KEY_STATE_PRESSED,
    KEY_STATE_HOLD,
    KEY_STATE_RELEASED,
};

#define KEY_JOY_UP      0x01
#define KEY_JOY_DOWN    0x02
#define KEY_JOY_LEFT    0x03
#define KEY_JOY_RIGHT   0x04
#define KEY_JOY_CENTER  0x05
#define KEY_BTN_LEFT1   0x06
#define KEY_BTN_RIGHT1  0x07

#define KEY_BACKSPACE   0x08 
#define KEY_TAB         0x09
#define KEY_ENTER       0x0A 
// 0x0D - CARRIAGE RETURN
#define KEY_BTN_LEFT2   0x11
#define KEY_BTN_RIGHT2  0x12


#define KEY_MOD_ALT     0x1A
#define KEY_MOD_SHL     0x1B
#define KEY_MOD_SHR     0x1C
#define KEY_MOD_SYM     0x1D
#define KEY_MOD_CTRL    0x7E

#define KEY_ESC       0xB1
#define KEY_UP        0xb5
#define KEY_DOWN      0xb6
#define KEY_LEFT      0xb4
#define KEY_RIGHT     0xb7

#define KEY_BREAK     0xd0 // == KEY_PAUSE
#define KEY_INSERT    0xD1
#define KEY_HOME      0xD2
#define KEY_DEL       0xD4
#define KEY_END       0xD5
#define KEY_CAPS_LOCK   0xC1

#define KEY_F1 0x81
#define KEY_F2 0x82
#define KEY_F3 0x83
#define KEY_F4 0x84
#define KEY_F5 0x85
#define KEY_F6 0x86
#define KEY_F7 0x87
#define KEY_F8 0x88
#define KEY_F9 0x89
#define KEY_F10 0x90

typedef void (*key_callback)(char, enum key_state);
typedef void (*lock_callback)(bool, bool);

void keyboard_process(void);
void keyboard_set_key_callback(key_callback callback);
void keyboard_set_lock_callback(lock_callback callback);
bool keyboard_get_capslock(void);
bool keyboard_get_numlock(void);
void keyboard_init(void);

#define NUM_OF_COLS 8
#define NUM_OF_ROWS 7

#define NUM_OF_BTNS 12

#define KEY_POLL_TIME 10

#define KEY_LIST_SIZE 10


#define KEY_HOLD_TIME 300

#endif
