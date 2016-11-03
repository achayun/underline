#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define vt_ESC "\x1b["

extern ssize_t ul_write(int fd, void *buf, size_t count);

void vt_move_cursor(int fd, unsigned int col)
{
	if (col > 99)
		col = 99;
	char *ip;
#define VT_LEGACY
#ifdef VT_LEGACY
    //Legacy terminals (Hyper-terminal) do not support the 'G' command
    //to go to the beginning of line we send ESC[#D
    //(brings the cursor back). We assume 200 is above max width
    char seq[] = vt_ESC"200D" vt_ESC"XXC";
    ip = seq + 8;
#else
    char seq[] = vt_ESC"0G" vt_ESC"XXC";
	ip = seq + 6;
#endif
	unsigned int d = col / 10;
	*ip = '0' + d;
	*(ip + 1) = '0' + (col - (d * 10));
    ul_write(fd, seq, sizeof(seq));
}

void vt_erase_from_cursor(int fd)
{
    ul_write(fd, vt_ESC"0K", 4);
}

void vt_clear_screen(int fd)
{
	ul_write(fd, vt_ESC"H" vt_ESC"2J",7);
}

void vt_set_termios(int fd, struct termios *stored_settings)
{
    /* change the terminal settings to return each character as it is typed
     * (disables line-oriented buffering)
     * Originally taken from:
     * http://www.linuxquestions.org/questions/programming-9/unbuffered-stdin-176039/ */

    // obtain the current settings flags, to be restored when we're done
    tcgetattr(fd, stored_settings);

    struct termios new_settings;
    memcpy(&new_settings, stored_settings, sizeof(struct termios));

    /* modify flags: disable canonical mode, echo,
     * extended chars and signal handling */
    new_settings.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG);

    /* diable input modes: no break, no CR to NL, no parity check,
     * no strip char,no start/stop output control. */
    new_settings.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    new_settings.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    new_settings.c_cflag |= (CS8);

    /* vtime and vmin setting interactions:
     * both > 0
     * Blocks until first new character, then tries to get a total vmin
     * characters, but never waits more than vtime between characters.
     * Returns when got vmin chars or the wait timeout.
     * vtime = 0, vmin > 0
     * Blocks until vmin characters received (or a signal is received)
     * vtime > 0, vmin = 0
     * If a character is ready within vtime, it is returned immediately.
     * If no character is avalable within vtime, zero is returned.
     * both = 0
     * Documentation somewhat unclear, but apparently returns immediately
     * with all available characters up to the limit of the number
     * requested by a read(). Returns -1 if no characters are available.  */
    new_settings.c_cc[VTIME] = 1; // timeout (tenths of a second)
    new_settings.c_cc[VMIN] = 0; // minimum number of characters

    // apply the new settings
    tcsetattr(fd, TCSANOW, &new_settings);

    /* note:
     * The return value from tcsetattr() is not tested because its value
     * reflects "success" if any PART of the attributes is changed, not
     * when all the values are changed as requested (stupidity!).
     * Since the content of the termios structure may differ with
     * implementation, as may the various constants such as ICANON, I see
     * no elegant way to check if the desired actions were completed
     * successfully. Comparing byte-by-byte shows the current state is
     * NOT EQUAL to the requested state, and yet it runs, so the changes
     * were apparently made. Can not check for success/failure.   */
}

void vt_restore_termios(int fd, const struct termios* stored_settings)
{
    tcsetattr(fd, TCSANOW, stored_settings);
}

