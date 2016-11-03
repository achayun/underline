#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "underline.h"

int main(void)
{
    while (1) {
        char *line = readline("$ ");
        if (! strcmp(line, "exit"))
            break;

        system(line);
    }
    return 0;
}
