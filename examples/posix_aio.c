#include <aio.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "underline.h"

#define PROMPT "$ "

void aio_completion_handler(sigval_t sigval);
void line_done_handler(char *line);
void alarm_handler(int signum) {}

int running = 1;
int main(void)
{
    /* Setup AIO notification. For more AIO examples see:
     * http://www.ibm.com/developerworks/linux/library/l-async/ */

    /* Read more about AIO to pick the buffer size right for you */
    char aio_buf[10];

    struct aiocb aiocb = {
        .aio_fildes   = STDIN_FILENO,
        .aio_buf      = &aio_buf,
        .aio_nbytes   = sizeof(aio_buf),
        .aio_sigevent = {
            .sigev_notify            = SIGEV_THREAD,
            .sigev_notify_function   = aio_completion_handler,
            .sigev_notify_attributes = NULL,
            .sigev_value.sival_ptr   = &aiocb
        },
    };

    /* SIGALRM will be used to wake the main loop
     * The alarm handler can be empty, as pause() returns
     * after a signal has been handled
     */
    signal(SIGALRM, alarm_handler);

    /* Setup Underline for aysnc callback */
    ul_init(STDIN_FILENO);
    ul_set_readline_cb(line_done_handler);

    /* Show prompt for the first time */
    ul_start_line(PROMPT);

    /* Kick aio read for the first time */
    int ret;
    ret = aio_read(&aiocb);
    if (ret != 0) {
        perror("aio_read");
        return -1;
    }

    {
        /* Here you will want to put the main loop of the code */
        /* ... */

        /* Nothing to do, just wait for a aignal to wake us up */
        pause();
        /* Note: We could use any signalling method here.
         * for example, see aio_suspend */
    }

    /* Finalize */
    aio_cancel(STDIN_FILENO, &aiocb);
    ul_finalize();
    return 0;
}

static inline void quit(void)
{
    kill(getpid(), SIGALRM);
}

void aio_completion_handler(sigval_t sigval)
{
    struct aiocb *req;

    req = (struct aiocb *)sigval.sival_ptr;
    int status = aio_error(req);
    if (status == 0) {

        /* Request completed successfully, get data */
        int len = aio_return( req );
        char *p = (char *)req->aio_buf;
        while (len--)
            ul_got_char(*(p++));

        /* Done handling this event, schedule the next one*/
        int ret = aio_read(req);
        if (ret != 0) {
            perror("aio_read");
            quit();
            return;
        }

    } else if (status == -1) {
        perror("aio_completion handler");
    }
    /* Other values of status are ignored */
}

void line_done_handler(char *line)
{
    if (! strcmp(line, "quit") || ! strcmp(line, "exit") ) {
        quit();
        return;
    }
    if (! strcmp(line, "echo off"))
        ul_set_echo_off(1);
    if (! strcmp(line, "echo on"))
        ul_set_echo_off(0);
    printf("line: [%s]\r\n", line);
    ul_start_line(PROMPT);
}
