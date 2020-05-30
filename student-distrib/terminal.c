#include "terminal.h"

uint32_t tid = 0;
tty_t ttys[MAX_TERMINALS];
uint8_t in_shell = FALSE;

/*
  init_terminals
	description: initialized the terminals and assigns then to their
               own vidmem and starts at shell 0
	inputs: None
	output: None
*/
void init_terminals(void) {
    int32_t i;
    uint8_t bg_colors[] = {BLACK, BLACK, BLACK};
    uint8_t fg_colors[] = {BRIGHT_CYAN, WHITE, BRIGHT_RED};
    // Initialise all terminal vars
    for (i = -1; i < MAX_TERMINALS; i++) {
        ttys[i].vidmem_ptr = get_vidmem_tty(i);
        ttys[i].cursor_x = 0;
        ttys[i].cursor_y = 0;
        ttys[i].insert_mode = 1;
        ttys[i].current_size = 0;
        ttys[i].cursor_pos = 0;
        ttys[i].read_pending = FALSE;
        ttys[i].returned = FALSE;
        ttys[i].is_visible = FALSE;
        ttys[i].background_color = bg_colors[i];
        ttys[i].foreground_color = fg_colors[i];
        if (ENABLE_HISTORY) {
            ttys[i].history_pos = 0;
            ttys[i].history_viewer = 0;
			ttys[i].history_size = 0;
        }
		set_vidmem(i);
	    clear();
    }
	// Switch to first terminal
    ttys[tid].is_visible = TRUE;
    background_color = ttys[tid].background_color;
    foreground_color = ttys[tid].foreground_color;
	set_vidmem(tid);
	map_addr_to_addr(ttys[tid].vidmem_ptr, (uint8_t*)VIDEO);
	clear();
}

/*
  terminal_switch
	description: switch from one terminal to another
	inputs: new_tid - shell/terminal id to switch to
	output: 0 if switch was succesful, -1 on error
*/
int32_t terminal_switch(uint32_t new_tid) {
    if (new_tid == tid) return 0;  // No need to switch
    if (new_tid >= MAX_TERMINALS || ttys[new_tid].vidmem_ptr == NULL) return -1;
	// Save current terminal vars
    get_vidmem(tid);
	// Set which tty is visible, and change colors
	ttys[tid].is_visible = FALSE;
	ttys[new_tid].is_visible = TRUE;
	// Un-map vidmem
	map_addr_to_addr(ttys[tid].vidmem_ptr, ttys[tid].vidmem_ptr);
    // Memcpy vidmem to actual vga vidmem
    memcpy(ttys[tid].vidmem_ptr, (uint8_t*)VIDEO, 2*NUM_COLS*NUM_ROWS);
    memcpy((uint8_t*)VIDEO, ttys[new_tid].vidmem_ptr, 2*NUM_COLS*NUM_ROWS);
	// Map new tid to vidmem
	map_addr_to_addr(ttys[new_tid].vidmem_ptr, (uint8_t*)VIDEO);
	set_vidmem(new_tid);
    // Load new terminal's vars
    setcursor(ttys[new_tid].cursor_x, ttys[new_tid].cursor_y);
    setmode(ttys[new_tid].insert_mode);
    tid = new_tid;
    return 0;
}

/*
  terminal_write
	description: syscall for stout
	inputs: buff - char buff to be printed
          size - max buff size to print
          fd - not used
	output: bytes written to stout
*/
int32_t terminal_write(int32_t fd, int8_t* buff, uint32_t size){
    if (buff == NULL || NUM_FILES <= fd || fd < 0) return -1;
    int32_t bytes_written;
    for (bytes_written = 0; bytes_written < size; bytes_written++) {
        putc(buff[bytes_written]);
    }
    return bytes_written;
}

/*
  terminal_read
	description: syscall for stdin
	inputs: buff - char buff to be filled with terminal input
          size - size of buff in bytes
          fd - not used
	output: bytes read from stdin
*/
int32_t terminal_read(int32_t fd, int8_t* buff, uint32_t size){
    if (buff == NULL || NUM_FILES <= fd || fd < 0) return -1;
	int terminal_id = get_current_pcb()->tid;
    ttys[terminal_id].read_pending = TRUE;

    // Wait for newline
    sti();
    while (!ttys[terminal_id].returned);
    // Once we get our return, we ignore interrupts
    cli();

    ttys[terminal_id].returned = FALSE;
    // Copy keyboard buffer to external buffer
    int32_t i;
    for (i = 0; (i < size) && (i < ttys[terminal_id].current_size); i++) {
		    buff[i] = ttys[terminal_id].keyboard_buff[i];
	  }
    if (ENABLE_HISTORY && in_shell) {
        // To avoid repetition, we only add the command if it's != than the last command
        uint32_t prev_idx = (ttys[terminal_id].history_pos == 0) ? HISTORY_LENGTH-1 : ttys[terminal_id].history_pos-1;
        int retval = strncmp((int8_t*)ttys[terminal_id].command_history[prev_idx], (int8_t*)ttys[terminal_id].keyboard_buff, ttys[terminal_id].current_size-1);
        if (retval != 0) {
            // Different command, add to history!
            ttys[terminal_id].commands_size[ttys[terminal_id].history_pos] = ttys[terminal_id].current_size-1;
            strncpy((int8_t*)ttys[terminal_id].command_history[ttys[terminal_id].history_pos], (int8_t*)ttys[terminal_id].keyboard_buff, ttys[terminal_id].current_size-1);
            ttys[terminal_id].history_pos++;
            ttys[terminal_id].history_pos %= HISTORY_LENGTH;
            ttys[terminal_id].history_size = (ttys[terminal_id].history_size != HISTORY_LENGTH-1) ? ttys[terminal_id].history_size+1 : HISTORY_LENGTH-1;
            // printf("Added! #%d\n", ttys[terminal_id].history_size);
        }
        ttys[terminal_id].history_viewer = ttys[terminal_id].history_pos;
    }
    ttys[terminal_id].read_pending = FALSE;
    // Clear buff
    ttys[terminal_id].current_size = 0;
    ttys[terminal_id].cursor_pos = 0;
    return i;
}
