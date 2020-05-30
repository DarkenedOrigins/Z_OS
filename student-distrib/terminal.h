#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "lib.h"
#include "types.h"
#include "keyboard.h"
#include "syscall.h"
#include "paging.h"
#include "colors.h"

#define HISTORY_LENGTH (20)

typedef struct {
    // Core functionality
    uint8_t* vidmem_ptr;
    int32_t cursor_x;
    int32_t cursor_y;
    int32_t insert_mode;
    int32_t current_size;
    int32_t cursor_pos;
    uint8_t keyboard_buff[KEYBOARD_BUFF_SIZE];
    volatile uint8_t read_pending;
    volatile uint8_t returned;
    uint8_t is_visible;
    uint8_t was_visited;
    // Extra (credit?) fancy stuff
    uint8_t background_color;
    uint8_t foreground_color;
    uint8_t command_history[HISTORY_LENGTH][KEYBOARD_BUFF_SIZE];
    uint32_t commands_size[HISTORY_LENGTH];
    uint32_t clear_num;  // How many bytes to clear on up/down
    uint32_t history_size;  // How many commands are saved
    uint32_t history_pos;  // index to next history 'slot'
    uint32_t history_viewer;  // user is looking at this one
} tty_t;

extern uint32_t tid;
extern tty_t ttys[MAX_TERMINALS];
extern uint8_t in_shell;

void init_terminals(void);
extern int32_t terminal_switch(uint32_t new_tid);
extern int32_t terminal_write(int32_t fd, int8_t* buff, uint32_t size);
extern int32_t terminal_read(int32_t fd, int8_t* buff, uint32_t size);

#endif
