#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define DEBUG(x) write(2, x, strlen(x))

int main() {
    char *t = "TEST\aTEST\n";

    DEBUG("Test #1\n");
    printf("%s", t);

    DEBUG("Test #2\n");
    fprintf(stdout, "%s", t);

    DEBUG("Test #3\n");
    fwrite(t, strlen(t), 1, stdout);

    DEBUG("Test #4\n");
    fputs(t, stdout);

    DEBUG("Test #5\n");
    puts(t);

    DEBUG("Test #6\n");
    write(1, t, strlen(t));

    DEBUG("Test #7\n");
    putc('\a', stdout);

    DEBUG("Test #8\n");
    putchar('\a');

    DEBUG("Test #9\n");
    fputc('\a', stdout);


    return 0;
}
