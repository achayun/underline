/* Underline core functionality public API */

#ifndef _UNDERLINE_H_
#define _UNDERLINE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*readline_cb)(char *line);

#define LINE_MAX 128

#define HIST_SIZE 16

struct history {
    char buf[HIST_SIZE][LINE_MAX];
    int first;
    int last;
    int current;
};

struct ul_state;    //Opeque library state struct. See source for details

typedef int (*ul_action_cb)(struct ul_state *s);

void ul_init(int fd);
void ul_finalize();
void ul_set_echo_off(int echo);
void ul_start_line(const char *prompt);
void ul_got_char(char c);
void ul_set_write_cb(ssize_t (*func)(int, const void *, size_t));
ssize_t ul_write(int, const void *, size_t);
void ul_set_readline_cb(readline_cb func);
//TODO: int ul_map(const char *keyseq, int len, ul_action_cb func);

char *readline(const char *prompt);

void hist_add(const char *line, int len);
void hist_prev(char *line, int *len);
void hist_next(char *line, int *len);
void hist_current_clear_pos();

#ifdef __cplusplus
}
#endif

#endif /*_UNDERLINE_H_*/
