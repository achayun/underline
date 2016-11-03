/* Underline VT-100 compatible terminals basic actions */

#ifndef _VT_H_
#define _VT_H_

struct termios;
void vt_move_cursor(int fd, int col);
void vt_erase_from_cursor(int fd);
void vt_clear_screen(int fd);
void vt_set_termios(int fd, struct termios *stored_settings);
void vt_restore_termios(int fd, const struct termios *stored_settings);

#endif /*_VT_H_*/
