/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab */

#include "lib.h"
#include "terminal.h"

#define ATTRIB      (0x7)
#define BG_CLEAR    (0x8F)
#define FG_CLEAR    (0x70)

static int screen_x;
static int screen_y;
char* video_mem = (char *)VIDEO;
uint8_t background_color = BLACK;
uint8_t foreground_color = GRAY;
bool is_visible = TRUE;

#define attribute_byte (background_color << 4 | foreground_color)

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory
 */
void clear(void) {
    memset_word(video_mem, (attribute_byte << 8) | BLANK_CHAR, NUM_COLS*NUM_ROWS);
}

/* set_vidmem
 * Inputs: vidmem pointer
 * Return Value: 0 on success
 * Function: setter function for vidmem
 */
int32_t set_vidmem(int32_t terminal_id) {
    video_mem = (char*)ttys[terminal_id].vidmem_ptr;
	screen_x = ttys[terminal_id].cursor_x;
	screen_y = ttys[terminal_id].cursor_y;
	background_color = ttys[terminal_id].background_color;
	foreground_color = ttys[terminal_id].foreground_color;
	is_visible = ttys[terminal_id].is_visible;
    return 0;
}

/* get_vidmem
 * Inputs: vidmem pointer
 * Return Value: 0 on success
 * Function: getter function for vidmem
 */
int32_t get_vidmem(int32_t terminal_id) {
	ttys[terminal_id].vidmem_ptr = (uint8_t*)video_mem;
	ttys[terminal_id].cursor_x = screen_x;
	ttys[terminal_id].cursor_y = screen_y;
	ttys[terminal_id].background_color = background_color;
	ttys[terminal_id].foreground_color = foreground_color;
	ttys[terminal_id].is_visible = is_visible;
    return 0;
}

/* setbg
 * Inputs: color index 0-15
 * Return Value: none
 * Function: changes background color of vmem
 */
void setbg(unsigned int color) {
    if (color > 15) return;
    background_color = color;
    color <<= 4;
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1) + 1) &= BG_CLEAR;
        *(uint8_t *)(video_mem + (i << 1) + 1) |= color;
    }
}

/* setfg
 * Inputs: color index 0-15
 * Return Value: none
 * Function: changes foreground color of vmem
 */
void setfg(unsigned int color) {
    if (color >= NUM_COLORS) return;
    foreground_color = color;
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1) + 1) &= FG_CLEAR;
        *(uint8_t *)(video_mem + (i << 1) + 1) |= color;
    }
}

/* reset_colors
 * Inputs: none
 * Return Value: none
 * Function: changes fore/background colors to default
 */
void reset_colors(void) {
    foreground_color = GRAY;
    background_color = BLACK;
}

/* backspace
 * Inputs: none
 * Return Value: none
 * Function: removes the last character printed to screen */
void backspace(void) {
    // Check bounds
    if (screen_x > 0) {
        screen_x -= 1;
    } else if (screen_y > 0) {
        screen_y -= 1;
        screen_x = NUM_COLS - 1;
    }
    int position = (screen_y*NUM_COLS) + screen_x;
    *(uint8_t *)(video_mem + (position << 1)) = BLANK_CHAR;
    *(uint8_t *)(video_mem + (position << 1) + 1) = attribute_byte;
    setcursor(screen_x, screen_y);
}

/* rbackspace
 * Inputs: none
 * Return Value: none
 * Function: repeatedly backspaces */
void rbackspace(uint32_t n) {
    uint32_t i;
    for (i = 0; i < n; i++)
      backspace();
}

/* clear_line
 * Inputs: line_num the ine number to be cleared
 * Return Value: none
 * Function: Clears one row of vmem */
void clear_line(unsigned int line_num) {
    // Clear line_num row
    if (line_num >= NUM_ROWS) return;
    uint8_t* start_addr = (uint8_t*)(video_mem + ((line_num*NUM_COLS) << 1));
    memset_word(start_addr, (attribute_byte << 8) | BLANK_CHAR, NUM_COLS);
}

/* scroll
 * Inputs: none
 * Return Value: none
 * Function: moves the vmem up by one, making space at the bottom */
void scroll(void) {
    int x, y, src, dest;
    // Move everything up one, erasing the top row
    for (y = 0; y < NUM_ROWS-1; y++) {
        for (x = 0; x < NUM_COLS; x++) {
            dest = (y*NUM_COLS) + x;
            src = dest + NUM_COLS;
            *(uint8_t *)(video_mem + (dest << 1)) = *(uint8_t *)(video_mem + (src << 1));
            *(uint8_t *)(video_mem + (dest << 1) + 1) = *(uint8_t *)(video_mem + (src << 1) + 1);
        }
    }
    clear_line(NUM_ROWS-1);
}

/* movecursor
 * Inputs: dir = direction of movement
 * Return Value: none
 * Function: moves the writing cursor */
void movecursor(int dir) {
    int x = screen_x;
    int y = screen_y;
    switch (dir) {
        case UP:    y--;  break;
        case DOWN:  y++;  break;
        case LEFT:  x--;  break;
        case RIGHT: x++;  break;
    }
    if (x < 0) {
        y--;
        x = NUM_COLS-1;
    } else if (x >= NUM_COLS) {
        y++;
        x = 0;
    }
    // Check bounds
    if (y < 0 && y >= NUM_ROWS) return;
    // Change cursor position
    setcursor(x, y);
}

/* rmovecursor = repeated movecursor
 * Inputs: dir = direction of movement, n = amount
 * Return Value: none
 * Function: moves the writing cursor multiple times */
void rmovecursor(int dir, int n) {
    int i;
    for (i = 0; i < n; i++) {
        movecursor(dir);
    }
}

/* void setcursor(int x, int y);
 * Inputs: x and y - new position of the screen cursor
 * Return Value: none
 * Function: Sets the writing cursor
 * http://www.osdever.net/FreeVGA/vga/crtcreg.htm#0A */
void setcursor(int x, int y) {
    // Check bounds
    if (x < 0 && x >= NUM_COLS) return;
    if (y < 0 && y >= NUM_ROWS) return;
    // Change cursor position
    screen_x = x;
    screen_y = y;
	if (!is_visible) return;
    int position = (screen_y*NUM_COLS) + screen_x;
    // VGA Cursor Location Low Register
    outb(0x0F, VGA_REGISTER_PORT);
    outb((unsigned char)(position & 0xFF), VGA_DATA_PORT);

    // VGA Cursor Location High Register
    outb(0x0E, VGA_REGISTER_PORT);
    outb((unsigned char)((position >> 8) & 0xFF), VGA_DATA_PORT);
}

/* getcursor_x
 * Inputs: none
 * Return Value: screen_x
 * Function: getter function */
int getcursor_x(void) {
    return screen_x;
}

/* getcursor_y
 * Inputs: none
 * Return Value: screen_y
 * Function: getter function */
int getcursor_y(void) {
    return screen_y;
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc('%');
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc((uint8_t) *((int32_t *)esp));
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts(int8_t* s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc(s[index]);
        index++;
    }
    return index;
}

/* void putc(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 * Function: Output a character to the console */
void putc(uint8_t c) {
    if(c == '\n' || c == '\r') {
        if (++screen_y >= NUM_ROWS) {
            screen_y = NUM_ROWS-1;
            scroll();
        };
        screen_x = 0;
    } else {
        // Writes to actual vidmem
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = attribute_byte;
        if (++screen_x == NUM_COLS) {
          if (screen_y == NUM_ROWS-1) {
            scroll();
          } else {
            screen_y++;
          }
          screen_x = 0;
        } else {
          screen_x %= NUM_COLS;
          screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
        }
    }
    setcursor(screen_x, screen_y);
}

/* void putxya(uint8_t c, uint32_t x, uint32_t y, uint8_t attrib);
 * Inputs: uint_8* c = character to print
 *         x,y,a = x,y coordinates and attrib byte respectively
 * Return Value: void
 * Function: Output a character to the console */
void putxya(uint8_t c, uint32_t x, uint32_t y, uint8_t attrib) {
    if (x >= NUM_COLS || y >= NUM_ROWS) return;
    *(uint8_t *)(video_mem + ((NUM_COLS * y + x) << 1)) = c;
    *(uint8_t *)(video_mem + ((NUM_COLS * y + x) << 1) + 1) = attrib;
}

/* void putxy(uint8_t c, uint32_t x, uint32_t y);
 * Inputs: uint_8* c = character to print
 *         x,y = x,y coordinates
 * Return Value: void
 * Function: Output a character to the console */
void putxy(uint8_t c, uint32_t x, uint32_t y) {
    putxya(c, x, y, attribute_byte);
}

/* void putxy_fb(uint8_t c, uint32_t x, uint32_t y);
 * Inputs: uint_8* c = character to print
 *         x,y = x,y coordinates
 * Return Value: void
 * Function: Output a character to the console */
void putxy_fb(uint8_t c, uint32_t x, uint32_t y, uint8_t fg, uint8_t bg) {
    if (fg >= NUM_COLORS) return;
    if (bg >= NUM_COLORS) return;
    putxya(c, x, y, bg << 4 | fg);
}

/* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
 * Inputs: uint32_t value = number to convert
 *            int8_t* buf = allocated buffer to place string in
 *          int32_t radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix) {
    static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int8_t *newbuf = buf;
    int32_t i;
    uint32_t newval = value;

    /* Special case for zero */
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    /* Go through the number one place value at a time, and add the
     * correct digit to "newbuf".  We actually add characters to the
     * ASCII string from lowest place value to highest, which is the
     * opposite of how the number should be printed.  We'll reverse the
     * characters later. */
    while (newval > 0) {
        i = newval % radix;
        *newbuf = lookup[i];
        newbuf++;
        newval /= radix;
    }

    /* Add a terminating NULL */
    *newbuf = '\0';

    /* Reverse the string and return */
    return strrev(buf);
}

/* int8_t* strrev(int8_t* s);
 * Inputs: int8_t* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
int8_t* strrev(int8_t* s) {
    register int8_t tmp;
    register int32_t beg = 0;
    register int32_t end = strlen(s) - 1;

    while (beg < end) {
        tmp = s[end];
        s[end] = s[beg];
        s[beg] = tmp;
        beg++;
        end--;
    }
    return s;
}

/* uint32_t strlen(const int8_t* s);
 * Inputs: const int8_t* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
uint32_t strlen(const int8_t* s) {
    register uint32_t len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

/* int32_t strstrip(int8_t* s);
 * Inputs: int8_t* s = string remove trailing and leading spaces to
 * Return Value: Number of spaces removed
 * Function: return length of string s */
int32_t strstrip(int8_t* s) {
    if (s == NULL) return -1;
    register uint32_t leading = 0;
    register uint32_t i = 0;
    uint32_t len = strlen(s);
    // Figure out how many leading spaces are there
    while (s[leading] == ' ')
        leading++;
    // Shift the str left, overwritting the spaces
    for (i = 0; i < len; i++) {
        s[i] = s[i+leading];
    }
    // Add a null terminator after all the text, loop from the end.
    len -= leading;
    for (i = len-1; (i > 0) && (s[i] == ' '); i--);
    s[i+1] = '\0';
    return leading+len-i-1;
}

/* int32_t strsplit(int8_t* s);
 * Inputs: int8_t* s = replaces spaces with nulls
 *         strip should be called first.
 * Ex: "a  bc d" -> "a..bc.d" (.=\0)
 * Return Value: Number of words found (3 in the ex)
 * Function: splits a string into words */
int32_t strsplit(int8_t* s) {
    if (s == NULL) return -1;
    register uint32_t i = 0;
    register uint32_t word_num = 0;
    register uint32_t in_word = FALSE;

    while (s[i] != '\0') {
        if (s[i] == ' ') {
            in_word = FALSE;
            s[i] = '\0';
        } else if (!in_word) {
            in_word = TRUE;
            word_num++;
        }
        i++;
    }
    return word_num;
}

/* int32_t strgetword(uint32_t n, int8_t* s, uint32_t size);
 * Inputs: int8_t* s = string to get nth word from
 *         uint32_t n = word number you want (0 indexed)
 *         uint32_t size = size of full string
 * Ex: (2, "a  bc d", 7) -> "d"
 * Return Value: NULL on error, else word
 * Function: gets the nth word of a string (needs to be split) */
int8_t* strgetword(uint32_t n, int8_t* s, uint32_t size) {
    if (s == NULL) return NULL;
    register uint32_t i = 0;
    register int32_t word_num = -1;
    register uint32_t in_word = FALSE;

    while (i < size) {
        if (s[i] == '\0') {
            in_word = FALSE;
        } else if (!in_word) {
            in_word = TRUE;
            word_num++;
        }
        if ((uint32_t)word_num == n) {
            return &s[i];
        }
        i++;
    }
    return NULL;
}

/* void* memset(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* memset(void* s, int32_t c, uint32_t n) {
    c &= 0xFF;
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
            :
            : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_word(void* s, int32_t c, uint32_t n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to value c */
void* memset_word(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_dword(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void* memset_dword(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void* memcpy(void* dest, const void* src, uint32_t n) {
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
            :
            : "S"(src), "D"(dest), "c"(n)
            : "eax", "edx", "memory", "cc"
    );
    return dest;
}

/* void* memmove(void* dest, const void* src, uint32_t n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              uint32_t n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void* memmove(void* dest, const void* src, uint32_t n) {
    asm volatile ("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
            :
            : "D"(dest), "S"(src), "c"(n)
            : "edx", "memory", "cc"
    );
    return dest;
}

/* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
 * Inputs: const int8_t* s1 = first string to compare
 *         const int8_t* s2 = second string to compare
 *               uint32_t n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n) {
    int32_t i;
    for (i = 0; i < n; i++) {
        if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

            /* The s2[i] == '\0' is unnecessary because of the short-circuit
             * semantics of 'if' expressions in C.  If the first expression
             * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
             * s2[i], then we only need to test either s1[i] or s2[i] for
             * '\0', since we know they are equal. */
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
int8_t* strcpy(int8_t* dest, const int8_t* src) {
    int32_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 *                uint32_t n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n) {
    int32_t i = 0;
    while (src[i] != '\0' && i < n) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        video_mem[i << 1]++;
    }
}

/* void window
 * Inputs: name - window name
 *         then window coordinates
 * Return Value: none
 * Function: draw window */
void window(const int8_t* name, uint32_t top_left_x, uint32_t top_left_y,
               uint32_t bottom_right_x, uint32_t bottom_right_y, uint8_t double_border) {
    if (double_border >= 2) return;
    if (top_left_x < 0 || top_left_x >= NUM_COLS) return;
    if (top_left_y < 0 || top_left_y >= NUM_ROWS) return;
    if (bottom_right_x < 0 || bottom_right_x >= NUM_COLS) return;
    if (bottom_right_y < 0 || bottom_right_y >= NUM_ROWS) return;
    if (top_left_x >= bottom_right_x || top_left_y >= bottom_right_y) return;

    uint32_t fonts[] = {0xB3, 0xC4, 0xDA, 0xD9, 0xC0, 0xBF, 0xB4, 0xC3,
                        186, 205, 201, 188, 200, 187, 185, 204};
    uint32_t width = bottom_right_x - top_left_x;
    uint32_t height = bottom_right_y - top_left_y;
    uint32_t name_length = (name == NULL) ? 0 : strlen(name);
    uint32_t offset = (double_border == TRUE) ? 8 : 0;
    int8_t name_cpy[NUM_COLS];
    strncpy(name_cpy, name, NUM_COLS);

    uint32_t i;
    // Draw sides first
    for (i = top_left_y+1; i < top_left_y + height; i++) {
        putxy(fonts[0+offset], top_left_x, i);
        putxy(fonts[0+offset], top_left_x+width, i);
    }
    // Draw top and bottom
    for (i = top_left_x+1; i < top_left_x + width; i++) {
        putxy(fonts[1+offset], i, top_left_y);
        putxy(fonts[1+offset], i, top_left_y+height);
    }
    // Draw corners
    putxy(fonts[2+offset], top_left_x, top_left_y);
    putxy(fonts[3+offset], bottom_right_x, bottom_right_y);
    putxy(fonts[4+offset], top_left_x, top_left_y+height);
    putxy(fonts[5+offset], top_left_x+width, top_left_y);
    // Draw name
    if (name_length > 0) {
        if (name_length > width-6) {
            name_cpy[width-8] = '.';
            name_cpy[width-7] = '.';
            name_cpy[width-6] = '.';
            name_cpy[width-5] = '\0';
            name_length = width-6;
        }
        uint32_t name_start_x = top_left_x + width/2 - name_length/2;
        int x, y;
        x = getcursor_x();
        y = getcursor_y();
        setcursor(name_start_x-2, top_left_y);
        putc(fonts[6+offset]);
        putc(' ');
        puts(name_cpy);
        putc(' ');
        putc(fonts[7+offset]);
        setcursor(x, y);
    }
}
