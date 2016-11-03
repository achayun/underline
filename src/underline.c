#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>

#include "vt.h"
#include "underline.h"

struct ul_state {
    int fd;
    char buf[LINE_MAX];
    int pos;
    int len;
    char *prompt;
    int plen;

    int edit;
    int control_state;
    int echo_off;

    ssize_t (*read)(int, void *, size_t);
    ssize_t (*write)(int, const void *, size_t);

    //User callbacks
    readline_cb rl_cb;

    //History. TODO: Allow custom memory allocator/arena instead of static allocation
    struct history history;
};

enum {
    CONTROL_STATE_START = 0,
    CONTROL_STATE_ESC,
    CONTROL_STATE_ESC_OB,
};

static struct ul_state state = {0};
static struct termios stored_settings;

static int handle_control_char(const char c);
static void ul_refresh_line(int fd);

void ul_init(int fd)
{
    memset(&state, 0, sizeof(state));
    state.fd = fd;
    state.read = read;
    state.write = write;
    if (isatty(state.fd))
        vt_set_termios(state.fd, &stored_settings);
}

void ul_finalize()
{
    if (isatty(state.fd))
        vt_restore_termios(state.fd, &stored_settings);
}

void ul_set_echo_off(int echo)
{
	state.echo_off = echo;
}

void ul_set_readline_cb(readline_cb func)
{
    state.rl_cb = func;
}

void ul_set_write_cb(ssize_t (*func)(int, const void *, size_t))
{
    state.write = func;
}

ssize_t ul_write(int fd, const void *buf, size_t count)
{
    return state.write(fd, buf, count);
}
static int ul_show_prompt(int fd, const char *prompt)
{
    int len = strlen(prompt);
    state.write(fd, prompt, len);
    return len;
}

#define BELL "\x07"
static inline void beep(int fd)
{
    state.write(fd, BELL, 1);
}

/*
 * Synchronous version based on read from STDIN
 */
char *readline(const char *prompt)
{
    ul_init(STDIN_FILENO);
    ul_start_line(prompt);
    char c;
    while (state.edit) {
       while (state.read(state.fd, &c, 1) == 1)
           ul_got_char(c);
    }
    ul_finalize();
    return state.buf;
}

void ul_start_line(const char *prompt)
{
    state.len = 0;
    state.pos = 0;
    state.plen = ul_show_prompt(state.fd, prompt);
    state.edit = 1;
}

void ul_got_char(char c)
{
    if (handle_control_char(c))
        return;

    if (! isprint(c) || state.len == LINE_MAX) {
        beep(state.fd);
        return;
    }
    char *p = state.buf + state.pos;
    if (state.pos < state.len)
        memmove(p + 1, p, state.len - state.pos);

    *p = c;
    state.len++;
    ul_refresh_line(state.fd);
    state.pos++;

    //Workaround, cursor misplaced after editing
    if (!state.echo_off)
        vt_move_cursor(state.fd, state.pos + state.plen);
}

static inline void ul_line_done()
{
    state.edit = 0;
    state.buf[state.len] = '\0';
    state.write(state.fd, "\r\n", 2);
    state.control_state = CONTROL_STATE_START;

    //TODO: Register as rl_cb instead of hardcoded function call
    hist_add(state.buf, state.len);

    if (state.rl_cb)
        state.rl_cb(state.buf);
}

static inline void cursor_back()
{
    if (state.pos) state.pos--;
	if (!state.echo_off)
		vt_move_cursor(state.fd, state.pos + state.plen);
}
static inline void cursor_forward()
{
    if (state.pos< state.len) state.pos++;
	if (!state.echo_off)
		vt_move_cursor(state.fd, state.pos + state.plen);
}

static inline void cursor_home()
{
    state.pos = 0;
	if (!state.echo_off)
		vt_move_cursor(state.fd, state.pos + state.plen);
}

static inline void cursor_end()
{
    state.pos = state.len;
	if (!state.echo_off)
		vt_move_cursor(state.fd, state.pos + state.plen);
}

static inline void del_char_before()
{
    if (!state.pos || !state.len)
        return;
    memmove(state.buf + state.pos - 1, state.buf + state.pos, state.len - state.pos);
    state.pos--;
    state.len--;
    ul_refresh_line(state.fd);
}

//TODO: Wrap in an enum so users
//      reimplementing custom VT can choose their own control chars.
#define C_c '\03'
#define CR  '\x0d'
#define LF  '\x0a'
#define BS  '\x7f'  //Yes, \x7f is actually DEL, but almost all terms map the '<--- Backspace' key to del...

#define C_a '\x01'
#define C_b '\x02'
#define C_e '\x05'
#define C_f '\x06'
#define C_h '\x08'
#define ESC '\x1b'

//TODO: Move to a map. Hardcoded switch is bad
static int handle_control_char(const char c)
{
    switch(c) {
        case ESC:
            state.control_state = CONTROL_STATE_ESC;
            return 1;
        case '[':
            if (state.control_state == CONTROL_STATE_ESC) {
                state.control_state = CONTROL_STATE_ESC_OB;
                return 1;
            }
            else return 0;

        /* Arrows */
        case 'A':   /* Up */
            if (state.control_state == CONTROL_STATE_ESC_OB) {
                hist_prev(state.buf, &state.len);
                state.pos = 0;
                ul_refresh_line(state.fd);
                break;
            }
            else return 0;
        case 'B':   /* Down */
            if (state.control_state == CONTROL_STATE_ESC_OB) {
                hist_next(state.buf, &state.len);
                state.pos = 0;
                ul_refresh_line(state.fd);
                break;
            }
            else return 0;
        case 'C':   /* Right */
            if (state.control_state == CONTROL_STATE_ESC_OB) {
                cursor_forward();
                break;
            }
            else return 0;
        case 'D':   /* Left */
            if (state.control_state == CONTROL_STATE_ESC_OB) {
                cursor_back();
                break;
            }
            else return 0;
        case 'K':
        case 'F':   /* End */
            if (state.control_state == CONTROL_STATE_ESC_OB) {
                cursor_end();
                break;
            }
            else return 0;
        case 'H':   /* Home */
            if (state.control_state == CONTROL_STATE_ESC_OB) {
                cursor_home();
                break;
            }
            else return 0;
        /* Stop states */
        case C_c:
            state.pos = 0;
            state.len = 0;
            hist_current_clear_pos();
            /* fallthrough */
        case CR: /* fallthrough */
        case LF:
            ul_line_done();
            break;

        case BS: /* fallthrough */
        case C_h:
            del_char_before();
            break;

        //Cursor movements
        case C_a:
            cursor_home();
            break;
        case C_b:
            cursor_back();
            break;
        case C_e:
            cursor_end();
            break;
        case C_f:
            cursor_forward();
            break;
        default:
            state.control_state = CONTROL_STATE_START;
            return 0;
    }
    state.control_state = CONTROL_STATE_START;
    return 1;
}

static void ul_refresh_line(int fd)
{
	if (state.echo_off)
		return;

    //Move cursor to editing point
    vt_move_cursor(fd, state.pos + state.plen);

    //Write the rest of the line
    state.write(fd, &state.buf[state.pos], state.len - state.pos);

    vt_erase_from_cursor(fd);

    //Restore cursor
    vt_move_cursor(fd, state.pos + state.plen);
}

#define HIST_CYCLIC_INC(c) ( (c) = (c == HIST_SIZE - 1) ? 0 : (c) + 1)
#define HIST_CYCLIC_DEC(c) ( (c) = (c == 0) ? HIST_SIZE - 1 : (c) - 1)
void hist_add(const char *line, int len)
{
    if (len == 0)
        return;
    struct history *h = &state.history;
    memcpy(state.history.buf[h->last], line, len);
    HIST_CYCLIC_INC(h->last);
    if (h->first == h->last)
        HIST_CYCLIC_INC(h->first);
    h->current = h->last;
}

void hist_prev(char *line, int *len)
{
    struct history *h = &state.history;
    if (h->current == h->first)
        return;
    memcpy(h->buf[h->current], line, *len);
    h->buf[h->current][*len] = '\0';
    HIST_CYCLIC_DEC(h->current);
    *len = strlen(h->buf[h->current]);
    memcpy(line, h->buf[h->current], *len);
}

void hist_next(char *line, int *len)
{
    struct history *h = &state.history;
    if (h->current == h->last)
        return;
    memcpy(h->buf[h->current], line, *len);
    h->buf[h->current][*len] = '\0';
    HIST_CYCLIC_INC(h->current);
    *len = strlen(h->buf[h->current]);
    memcpy(line, h->buf[h->current], *len);
}

void hist_current_clear_pos()
{
    state.history.current = state.history.last;
}
