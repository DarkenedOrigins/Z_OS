#include "keyboard.h"
#include "terminal.h"

// Key mappings
int scan_map[SCAN_NUM] = {
  KEY_NONE, KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
  KEY_BACKSPACE, KEY_TAB, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',
  ']', '\n', KEY_CTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
  '\'', '`', KEY_LEFT_SHIFT, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.',
  '/', KEY_RIGHT_SHIFT, KEY_PRINT_SCREEN, KEY_ALT, ' ', KEY_CAPS_LOCK,
  KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
  KEY_NUM_LOCK, KEY_SCROLL_LOCK, KEY_HOME, KEY_UP_ARROW, KEY_PAGE_UP, KEY_KEYPAD_MINUS,
  KEY_LEFT_ARROW, KEY_KEYPAD_5, KEY_RIGHT_ARROW, KEY_KEYPAD_PLUS, KEY_END, KEY_DOWN_ARROW,
  KEY_PAGE_DOWN, KEY_INSERT, KEY_DELETE, KEY_NONE, KEY_NONE, KEY_NONE,
  KEY_F11, KEY_F12,
};

// Mapping used for shift+number row keys
char number_map[] = {')', '!', '@', '#', '$', '%', '^', '&', '*', '(',};


// Global vars for the status of modifier keys
uint8_t caps_locked = FALSE;
uint8_t num_locked = FALSE;
uint8_t scroll_locked = FALSE;
uint8_t lshift_pressed = FALSE;
uint8_t rshift_pressed = FALSE;
uint8_t ctrl_pressed = FALSE;
uint8_t alt_pressed = FALSE;

/*
  updateState
	description: Updates above state vars
	inputs: The character code that was just read
	output: returns 1 if a state has been updated, 0 else
	side effect: May modify state vars
*/
int updateState(int code){
    int has_updated = 1;

    // If a key has been pressed (as opposed to unpressed)
    if (code < UNPRESS_OFFSET) {

        // Set State variables (or toggle in the case of "lock" keys)
        switch (scan_map[code]) {
            case KEY_CAPS_LOCK:
                caps_locked = !caps_locked;
                break;
            case KEY_NUM_LOCK:
                // Pause Break is CTRL+NUMLOCK and doesnt enable numlock
                if (ctrl_pressed && KEY_DEBUG) {
                    printf("KEY_PAUSE_BREAK\n");
                } else {
                    num_locked = !num_locked;
                }
                break;
            case KEY_SCROLL_LOCK:
                scroll_locked = !scroll_locked;
            case KEY_LEFT_SHIFT:
                lshift_pressed = 1;
                break;
            case KEY_RIGHT_SHIFT:
                rshift_pressed = 1;
                break;
            case KEY_CTRL:
                ctrl_pressed = 1;
                break;
            case KEY_ALT:
                alt_pressed = 1;
                break;
            default:
                has_updated = 0;
        }
    /* A button has been unpressed */
    } else {
        code -= UNPRESS_OFFSET;
        // Set State variables
        switch (scan_map[code]) {
            case KEY_LEFT_SHIFT:
                lshift_pressed = 0;
                break;
            case KEY_RIGHT_SHIFT:
                rshift_pressed = 0;
                break;
            case KEY_CTRL:
                ctrl_pressed = 0;
                break;
            case KEY_ALT:
                alt_pressed = 0;
                break;
            default:
                has_updated = 0;
        }
    }
    return has_updated;
}

/*
  printBuff
  description: Helper function to print the char buffer
  inputs: none
	output: none
	side effect: Prints to vmem
 */
void printBuff(void) {
    int i, k;
    // Linearly display the buffer
    for (i = 0; i < ttys[tid].current_size; i++) {
        k = ttys[tid].keyboard_buff[i];
        putc((char)k);
    }
}

/*
  setmode
  description: Changes cursor to desired shape
               Text mode: Replace is 0, Insert is 1
  inputs: insert_mode
	output: none
	side effect: Changes cursor by writing to vga registers
 */
void setmode(uint8_t insert_mode) {
    if (insert_mode) {
        outb(VGA_CURSOR_REG, VGA_REGISTER_PORT);
        outb(VGA_CURSOR_INSERT, VGA_DATA_PORT);
    } else {
        outb(VGA_CURSOR_REG, VGA_REGISTER_PORT);
        outb(VGA_CURSOR_REPLACE, VGA_DATA_PORT);
    }
}

/*
  shortcutHandler
	description: Handles shortcuts such as CTRL-L
	inputs: The character code that was just read
	output: returns 1 if a shortcut has been executed, 0 else
	side effect: Varies depending on specific shortcut
*/
int shortcutHandler(int code){
    // Check code bounds
    if (!(0 <= code && code < SCAN_NUM)) return 0;
    int key = scan_map[code];

    /* Actual System Shortcuts */
    // CTRL-L Clears the screen and resert the cursor
    if (key == 'l' && ctrl_pressed && !alt_pressed) {
        setcursor(0,0);
        clear();
        if (COPY_ON_CLEAR) {
            printBuff();
            setcursor(0,0);
            rmovecursor(RIGHT, ttys[tid].cursor_pos);
        } else {
            ttys[tid].current_size = 0;
            ttys[tid].cursor_pos = 0;
        }
        return 1;
    }
    // CTRL-INSERT changes from insert mode to replace mode and back
    // and modifies the cursor shape to let the user know
    else if (key == KEY_INSERT && ctrl_pressed) {
        ttys[tid].insert_mode = !ttys[tid].insert_mode;
        setmode(ttys[tid].insert_mode);
        return 1;
    }
    // HOME moves the cursor to the start of the line
    else if (key == KEY_HOME && ENABLE_HOME_END) {
        rmovecursor(LEFT, ttys[tid].cursor_pos);
        ttys[tid].cursor_pos = 0;
        return 1;
    }
    // END moves the cursor to the end of the line
    else if (key == KEY_END && ENABLE_HOME_END) {
        rmovecursor(RIGHT, ttys[tid].current_size-ttys[tid].cursor_pos);
        ttys[tid].cursor_pos = ttys[tid].current_size;
        return 1;
    }
    // ALT+Fx to switch terminals (and shells)
    else if (key >= KEY_F1 && key <= KEY_F12 && alt_pressed) {
        uint32_t new_tid = (key - KEY_F1) >> KEY_FX_TO_NUM;   // Shift by 8 because non-printable keys are > 0xFF
        if (terminal_switch(new_tid) == 0) return 1;
    }
    // CTRL-C to force halt a process
    else if (key == 'c' && ctrl_pressed) {
        printf("Can't kill me!\n");
        //system_halt(255);
        return 1;
    }
	// Debug Shortcuts
	else if (key >= KEY_F1 && key <= KEY_F12 && ctrl_pressed && KEY_DEBUG) {
		uint32_t new_tid = (key - KEY_F1) >> KEY_FX_TO_NUM;
		terminal_write(1, (int8_t*)ttys[new_tid].keyboard_buff, ttys[new_tid].current_size);
	} else if (key == KEY_INSERT && ctrl_pressed && alt_pressed && KEY_DEBUG) {
		ttys[tid].history_pos = 0;
		ttys[tid].history_viewer = 0;
		ttys[tid].history_size = 0;
	}

    // Default
    return 0;
}

/*
  keypadHandler
	description: Handles keypad input. Only call if num_locked
	inputs: The character code that was just read
	output: returns correct key character, else 0
	side effect: None
*/
char keypadHandler(int code){
    if (!(0 <= code && code < SCAN_NUM)) return (char)0;
    int input_key = scan_map[code];
    if ((KEY_INSERT <= input_key && input_key <= KEY_ENTER) ||
         input_key == KEY_PRINT_SCREEN) {
        if (num_locked) {
            switch (input_key) {
                case KEY_END:          return '1';
                case KEY_DOWN_ARROW:   return '2';
                case KEY_PAGE_DOWN:    return '3';
                case KEY_LEFT_ARROW:   return '4';
                case KEY_KEYPAD_5:     return '5';
                case KEY_RIGHT_ARROW:  return '6';
                case KEY_HOME:         return '7';
                case KEY_UP_ARROW:     return '8';
                case KEY_PAGE_UP:      return '9';
                case KEY_INSERT:       return '0';
                case KEY_DELETE:       return '.';
                case KEY_PRINT_SCREEN: return '*';
                case KEY_KEYPAD_PLUS:  return '+';
                case KEY_KEYPAD_MINUS: return '-';
            }
        }
    }
    return (char) 0;
}

/*
  getPrintableKey
	description: if key is prinatble, return its correct character based on the
               current state vars. i.e: gets uppercase letters
  inputs: The character code that was just read
	output: returns correct key character, or null
	side effect: None
*/
char getPrintableKey(int code) {
    // If it's a printable character, handle it
    if (!(0 <= code && code < SCAN_NUM)) return 0;
    int input_key = scan_map[code];

    if (input_key & PRINTABLE_MASK && code < UNPRESS_OFFSET) {
        char character = (char) input_key;
        // Need to get uppercase version of letter
        if ((lshift_pressed || rshift_pressed) ^ caps_locked) {
            if ('a' <= character && character <= 'z' ) {
                return (char)(character - LETTER_UPPERCASE_OFFSET);
            }
        }
        // Get other symbol (i.e: 2 -> @)
        // Note: CapsLock doesn't work for symbols
        if (lshift_pressed || rshift_pressed) {
            if ('0' <= character && character <= '9') {
                return number_map[character - '0'];
            } else {
                switch ((char) character) {
                    case '\'': return '"';
                    case ',':  return '<';
                    case '-':  return '_';
                    case '.':  return '>';
                    case '/':  return '?';
                    case ';':  return ':';
                    case '=':  return '+';
                    case '[':  return '{';
                    case '\\': return '|';
                    case ']':  return '}';
                    case '`':  return '~';
                }
            }
        }
        // It may also not be 'uppercased' hence:
        return character;
    }
    // If it's  not a printable character, return 0
    return (char) 0;
}

/*
  moveBuff
	description: if possible, move contents of buffer over by one
               starting at, and including, pos. This makes room for
               a new char at pos (if moving right), or deletes it.
  inputs: the direction of movement (dir) and the start index (pos)
	output: returns 1 if moved buff, 0 else
	side effect: May modify keyboard_buff, cursor_pos, current_size
  function: This is used to shift chars when in insert mode
*/
int moveBuff(int dir, int pos) {
    int i;
	// Move buffer starting at 'pos' to the RIGHT
    if (dir == RIGHT && ttys[tid].current_size < KEYBOARD_BUFF_SIZE) {
        for (i = KEYBOARD_BUFF_SIZE-1; i > pos; i--) {
            ttys[tid].keyboard_buff[i] = ttys[tid].keyboard_buff[i-1];
        }
        ttys[tid].current_size++;
        ttys[tid].cursor_pos++;
        return 1;
	// Move buffer starting at 'pos' to the LEFT
    } else if (dir == LEFT) {
        for (i = pos; i < KEYBOARD_BUFF_SIZE-1; i++) {
            ttys[tid].keyboard_buff[i] = ttys[tid].keyboard_buff[i+1];
        }
        ttys[tid].current_size--;
        ttys[tid].cursor_pos--;
        return 1;
    }
    return 0;
}

/*
  addToBuff
	description: if key is prinatble, try to add it to the buffer
  inputs: The character that was just read
	output: returns 1 if added to buff, 0 else
	side effect: May modify keyboard_buff, cursor_pos, current_size
*/
int addToBuff(char character) {
    if (ttys[tid].insert_mode) {
        // Insert mode: insert chars in buffer by making room for them
        if (ttys[tid].current_size >= KEYBOARD_BUFF_SIZE) return 0;
        if (ttys[tid].cursor_pos == ttys[tid].current_size) {
			// If the cursor is at the end of the line, simply append
            ttys[tid].keyboard_buff[ttys[tid].cursor_pos++] = (char)character;
            ttys[tid].current_size++;
            return 1;
        } else {
			// If not at end, move buff to make room for new char
            int i;
            int x = getcursor_x();
            int y = getcursor_y();
            moveBuff(RIGHT, ttys[tid].cursor_pos);
            movecursor(RIGHT);
            for (i = ttys[tid].cursor_pos; i < ttys[tid].current_size; i++) {
                putc(ttys[tid].keyboard_buff[i]);
            }
            setcursor(x, y);
            ttys[tid].keyboard_buff[ttys[tid].cursor_pos-1] = (char)character;
            return 1;
        }
    } else {
        // Replace mode: overwritte chars in the buffer
        if (ttys[tid].cursor_pos < ttys[tid].current_size) {
			// Overwrite chars in buff by replacing them
            ttys[tid].keyboard_buff[ttys[tid].cursor_pos++] = (char)character;
            return 1;
        } else if (ttys[tid].current_size < KEYBOARD_BUFF_SIZE) {
			// Append to end of char
            ttys[tid].keyboard_buff[ttys[tid].cursor_pos++] = (char)character;
            ttys[tid].current_size++;
            return 1;
        }
    }
    return 0;
}

/*
  backspaceHandle
  description: Handles the backspace key press
  inputs: none
	output: returns 1 if a char was removed, 0 else
	side effect: May modify keyboard_buff, cursor_pos, current_size
 */
int backspaceHandle(void) {
    /* Note: The behavior of this function doesn't change depending
     *       on what text-mode we are using (replace/insert) */
    if (ttys[tid].current_size > 0) {
		if (ttys[tid].clear_num > 0) ttys[tid].clear_num--;
        // If at the end of a line, perform the normal backspace
        if (ttys[tid].cursor_pos == ttys[tid].current_size) {
            backspace();
            ttys[tid].current_size--;
            ttys[tid].cursor_pos--;
            return 1;
        // Otherwise we need to shift the buff and re-echo everything
        } else if (ttys[tid].cursor_pos > 0) {
            int i;
            int x = getcursor_x();
            int y = getcursor_y();
            moveBuff(LEFT, ttys[tid].cursor_pos-1);
            movecursor(LEFT);
            for (i = ttys[tid].cursor_pos; i < ttys[tid].current_size; i++) {
                putc(ttys[tid].keyboard_buff[i]);
            }
            putc(BLANK_CHAR);
            setcursor(x, y);
            movecursor(LEFT);
            return 1;
        }
    }
    return 0;
}

/*
  keyboard_handler_helper
	description: see keyboardHandler description
	inputs: none
	output: none
	side effect: Reads from keyboard and writes to screen and changes state vars
*/
void keyboard_handler_helper(void){
    // Get scan code from the keyboard
    uint8_t code = inb(KEYBOARD_SCAN_CODE_PORT);

    // First update state vars and check if its not a shortcut,
    // if it updates or is a shortcut, we are done
    int updated = updateState(code);
    int shortcuted = shortcutHandler(code);
    if (updated || shortcuted) return;

    // Key unpresses only matter for state vars & shortcuts
    if (code >= UNPRESS_OFFSET) return;

    // Check if its a 'normal' character
    int character = (int)getPrintableKey(code);

    if (ttys[tid].returned) return;

    // If its not a normal character, it might be from the keypad
    if (character == 0) {
        // Check if its from the keypad or a key unpress
        int num_char = (int)keypadHandler(code);
        if (num_char == 0) {
            // Handle Special keys (that weren't a shortcut)
            switch (scan_map[code]) {
                case KEY_BACKSPACE:
					// Map backspace to the correct function
                    backspaceHandle();
                    break;
                case KEY_TAB:
                    break;
                case KEY_RIGHT_ARROW:
					// Move the cursor to the right if possible
                    if (ttys[tid].cursor_pos < ttys[tid].current_size) {
                        movecursor(RIGHT);
                        ttys[tid].cursor_pos++;
                    }
                    break;
                case KEY_LEFT_ARROW:
					// Move the cursor to the left if possible
                    if (ttys[tid].cursor_pos > 0) {
                        movecursor(LEFT);
                        ttys[tid].cursor_pos--;
                    }
                    break;
                case KEY_UP_ARROW:
					// Move the cursor up if possible
                    if (ttys[tid].cursor_pos - NUM_COLS > 0) {
                        movecursor(UP);
                        ttys[tid].cursor_pos -= NUM_COLS;
					// Try going thru terminal history
                    } else if (ENABLE_HISTORY) {
						// If cursor isn't at the end, move it
						if (ttys[tid].cursor_pos != 0) {
							rmovecursor(RIGHT, ttys[tid].current_size-ttys[tid].cursor_pos);
							ttys[tid].cursor_pos = 0;
						}
                        rbackspace(ttys[tid].clear_num);
						ttys[tid].clear_num = 0;

                        ttys[tid].history_viewer = (ttys[tid].history_viewer != 0) ? ttys[tid].history_viewer-1 : ttys[tid].history_size-1;
                        ttys[tid].current_size = ttys[tid].commands_size[ttys[tid].history_viewer];
						ttys[tid].cursor_pos = ttys[tid].current_size;
                        int8_t* dest = (int8_t*) &ttys[tid].keyboard_buff;
                        int8_t* src = (int8_t*) &ttys[tid].command_history[ttys[tid].history_viewer];
                        strncpy(dest, src, ttys[tid].current_size);
                        ttys[tid].clear_num = ttys[tid].current_size;
                        printBuff();
                    }
                    break;
                case KEY_DOWN_ARROW:
					// Move the cursor down if possible
                    if (ttys[tid].cursor_pos + NUM_COLS <= ttys[tid].current_size) {
                        movecursor(DOWN);
                        ttys[tid].cursor_pos += NUM_COLS;
					// Try going thru terminal history
                    } else if (ENABLE_HISTORY) {
						if (ttys[tid].history_size == 0) break;
						if (ttys[tid].history_pos == ttys[tid].history_viewer) break;
						// If cursor isn't at the end, move it
						if (ttys[tid].cursor_pos != 0) {
							rmovecursor(RIGHT, ttys[tid].current_size-ttys[tid].cursor_pos);
							ttys[tid].cursor_pos = 0;
						}
                        rbackspace(ttys[tid].clear_num);
						ttys[tid].clear_num = 0;

                        ttys[tid].history_viewer++;
						if (ttys[tid].history_pos == ttys[tid].history_viewer) {
							ttys[tid].current_size = 0;
				            ttys[tid].cursor_pos = 0;
							break;
						}
                        ttys[tid].history_viewer %= ttys[tid].history_size;
                        ttys[tid].current_size = ttys[tid].commands_size[ttys[tid].history_viewer];
						ttys[tid].cursor_pos = ttys[tid].current_size;
						int8_t* dest = (int8_t*) &ttys[tid].keyboard_buff;
                        int8_t* src = (int8_t*) &ttys[tid].command_history[ttys[tid].history_viewer];
                        strncpy(dest, src, ttys[tid].current_size);
                        ttys[tid].clear_num = ttys[tid].current_size;
                        printBuff();
                    }
                    break;
            }
            return;
        } else {
            character = num_char;
        }
    }

    // Finally the correct char is in character.
    // Place it in the buffer and echo it.
    if (character == (int)'\n' || character == (int)'\r') {
        // If buffer is two lines and the cursor isn't on the bottom
        // line, movecursor right before as to avoid overwritting vmem
        rmovecursor(RIGHT, ttys[tid].current_size-ttys[tid].cursor_pos);
        // Print char, add to buff
        putc((char)character);
        ttys[tid].keyboard_buff[ttys[tid].current_size++] = (char) character;
        // Flush buffer (but not really, there's no point. Security?)
        if (ttys[tid].read_pending == FALSE)  {
            ttys[tid].current_size = 0;
            ttys[tid].cursor_pos = 0;
        } else {
            // Set entered flag for terminal read and wait for it to copy
            ttys[tid].returned = TRUE;
        }
		ttys[tid].clear_num = 0;
    } else {
        // If possible add to buff, otherwise ignore keypress
        if (addToBuff((char)character)) {
			ttys[tid].clear_num++;
			putc((char) character);
		}
    }
}

/*
  keyboardHandler
	description: Handles interrupts from the keyboard (key presses and releases)
	inputs: none
	output: none
	side effect: Reads from keyboard and writes to screen and changes state vars
*/
void keyboardHandler(void){
	cli(); // Don't want to stack keyboard interrupts

	// Make sure key stroke echos to active terminal
    get_vidmem(get_current_pcb()->tid);
	set_vidmem(tid);

	// Call the actual handler
	keyboard_handler_helper();

	// Change putc's routing back to the current job
	get_vidmem(tid);
	set_vidmem(get_current_pcb()->tid);

    // The syscall handler will IRET to restore interrupts for us
}

/*
  init_keyboard
	description: Initializes the keyboard
	inputs: none
	output: none
	side effect: Unmasks IRQ1
*/
void init_keyboard(void){
  	// //set all leds to off
  	// outb(SET_LEDS, KEYBOARD_SCAN_CODE_PORT);
  	// KeyboardHasAcked();
  	// outb(ALL_LEDS_OFF, KEYBOARD_SCAN_CODE_PORT);
  	// KeyboardHasAcked();
  	// //set scan code to 2
  	// outb(SET_SCAN_CODE, KEYBOARD_SCAN_CODE_PORT);
  	// KeyboardHasAcked();
  	// outb(KEYBOARD_SCAN_CODE, KEYBOARD_SCAN_CODE_PORT);
  	// KeyboardHasAcked();
  	// //set rate and Delay
  	// outb(SET_RATE_AND_DELAY, KEYBOARD_SCAN_CODE_PORT);
  	// KeyboardHasAcked();
  	// outb(DELAY_AND_REPEAT_RATE, KEYBOARD_SCAN_CODE_PORT);
  	// KeyboardHasAcked();
  	// //set keyboard to scanning so it sends interrupts
  	// outb(SET_SCANNING, KEYBOARD_SCAN_CODE_PORT);

  	// Unmask the keyboard's IRQ line
  	enable_irq(KEY_PIC_LINE);
}

/*
  KeyboardHasAcked
	description: waits for the keyboard to send an KEYBOARD_ACKNOWLEDGE
	inputs: none
	output: 1 if keyboard acknowledged the command or 0 if it needs a resend
	side effect: Reads from the keyboard scan code port
*/
int KeyboardHasAcked(void){
  	uint8_t KeyboardStatus;
  	while(1){
    		KeyboardStatus = inb(KEYBOARD_SCAN_CODE_PORT);	//gets keyboard status
    		if(KeyboardStatus == KEYBOARD_ACKNOWLEDGE){
    		    return 1;	//if keyboard ack return 1
    		}
    		if(KeyboardStatus == KEYBOARD_RESEND_CMD){
    		    return 0;	//if keyboard asks for a resend ret 0
    		}
  	}
}
