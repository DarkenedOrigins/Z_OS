#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "lib.h"
#include "i8259.h"
#include "types.h"

// If set to one we enable shortcut tests
#define KEY_DEBUG TRUE

// If set to one CTRL-L copies the current line to the top
#define COPY_ON_CLEAR TRUE

// If set to 1, enable the home/end shortcuts
#define ENABLE_HOME_END TRUE

// The total buffer size including newlines
#define TOTAL_KEYBOARD_BUFF_SIZE 128

// Enable keyboard history, using up/down arrows
#define ENABLE_HISTORY TRUE

// We are using Scan Codes from Set 1
// These can be seen at the following url
// http://www.vetra.com/scancodes.html

// These are non (easily) printable keys
// We ID them with hex values greater than
// a byte such that no conficts with normal keys occur
#define KEY_NONE 0x0000
#define KEY_BACKSPACE 0x0100
#define KEY_TAB 0x0200
#define KEY_CAPS_LOCK 0x0300
#define KEY_LEFT_SHIFT 0x0400
#define KEY_RIGHT_SHIFT 0x0500
#define KEY_ALT 0x0600
#define KEY_SPACE 0x0700
#define KEY_CTRL 0x0800
#define KEY_INSERT 0x0900
#define KEY_DELETE 0x0A00
#define KEY_LEFT_ARROW 0x0B00
#define KEY_HOME 0x0C00
#define KEY_END 0x0D00
#define KEY_UP_ARROW 0x0E00
#define KEY_DOWN_ARROW 0x0F00
#define KEY_PAGE_UP 0x1000
#define KEY_PAGE_DOWN 0x1100
#define KEY_RIGHT_ARROW 0x1200
#define KEY_NUM_LOCK 0x1300
#define KEY_KEYPAD_FSLASH 0x1400
#define KEY_KEYPAD_0 0x1500
#define KEY_KEYPAD_1 0x1600
#define KEY_KEYPAD_2 0x1700
#define KEY_KEYPAD_3 0x1800
#define KEY_KEYPAD_4 0x1900
#define KEY_KEYPAD_5 0x1A00
#define KEY_KEYPAD_6 0x1B00
#define KEY_KEYPAD_7 0x1C00
#define KEY_KEYPAD_8 0x1D00
#define KEY_KEYPAD_9 0x1E00
#define KEY_KEYPAD_ASTERISK 0x1F00
#define KEY_KEYPAD_DOT 0x2000
#define KEY_KEYPAD_MINUS 0x2100
#define KEY_KEYPAD_PLUS 0x2200
#define KEY_ENTER 0x2300
#define KEY_ESC 0x2400
#define KEY_F1 0x2500
#define KEY_F2 0x2600
#define KEY_F3 0x2700
#define KEY_F4 0x2800
#define KEY_F5 0x2900
#define KEY_F6 0x2A00
#define KEY_F7 0x2B00
#define KEY_F8 0x2C00
#define KEY_F9 0x2D00
#define KEY_F10 0x2E00
#define KEY_F11 0x2F00
#define KEY_F12 0x3000
#define KEY_PRINT_SCREEN 0x3100
#define KEY_SCROLL_LOCK 0x3200
#define KEY_PAUSE_BREAK 0x3300   //equivalent to ctrl+numlock

#define KEY_FX_TO_NUM (8)

// Key names array size
#define SCAN_NUM 90

// General Constants
#define UNPRESS_OFFSET 128
#define LETTER_UPPERCASE_OFFSET 32
#define PRINTABLE_MASK 0x00FF
#define KEYBOARD_BUFF_SIZE (TOTAL_KEYBOARD_BUFF_SIZE-1)

// Keyboard Init Constants
#define KEY_PIC_LINE  (0x01)
#define KEY_IRQ       (KEY_PIC_LINE + IRQ_OFFSET)

#define KEYBOARD_STATUS_PORT	(0x64)
#define KEYBOARD_SCAN_CODE_PORT	(0x60)

// Keyboard Commands
#define SET_LEDS				(0xED)
#define SET_SCAN_CODE			(0xF0)
#define SET_RATE_AND_DELAY		(0xF3)
#define SET_SCANNING			(0xF4)
#define ALL_LEDS_OFF			(0x00)
#define KEYBOARD_SCAN_CODE 		(1)
#define DELAY_AND_REPEAT_RATE	(0x00)

// Keyboard Replies
#define KEYBOARD_ACKNOWLEDGE	(0xFA)
#define KEYBOARD_RESEND_CMD		(0xFE)

// Keyboard Cursor Constants
#define VGA_CURSOR_REG          (0x0A)
#define VGA_CURSOR_INSERT       (0x0E)
#define VGA_CURSOR_REPLACE      (0x00)

// Shared vars:
// State Variables (CTRL, SHIFT...)
extern uint8_t num_locked;
extern uint8_t scroll_locked;
extern uint8_t lshift_pressed;
extern uint8_t rshift_pressed;
extern uint8_t ctrl_pressed;
extern uint8_t alt_pressed;

void init_keyboard(void);
void keyboardHandler(void);
void setmode(uint8_t insert_mode);

#endif
