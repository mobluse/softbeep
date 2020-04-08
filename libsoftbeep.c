/* Evil evil evil hack to get my speaker back to life
 * Copyright (C) 1998, 1999 Lennart Poettering <mz736f667462656570@poettering.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <curses.h>
#include <ctype.h>
#include <limits.h>

#include <X11/Xlib.h>

typedef struct FILE _IO_FILE;

/* BSDI has this functionality, but not define :() */
#if defined(RTLD_NEXT)
#define REAL_LIBC RTLD_NEXT
#else
#define REAL_LIBC ((void *) -1L)
#endif

#define BEL ('\a')
#define ESC ('\033')

//#define DEBUG

#ifdef DEBUG
static void debug(const char *TEMPLATE, ...) {
    char *p;
    va_list ap;

    va_start(ap, TEMPLATE);
    if (vasprintf(&p, TEMPLATE, ap) >= 0) {
        static ssize_t (*func) (int, const void*, size_t) = NULL;
        int fd;
        
        if (!func)
            func = (ssize_t (*) (int, const void*, size_t)) dlsym (REAL_LIBC, "write");

        if ((fd = open("/tmp/sb-debug", O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR)) >= 0) {
            func(fd, p, strlen(p));
            fsync(fd);
            close(fd);
        }
        
        free(p);
    }
    va_end(ap);
}
#endif

static int remove_bel = -1;

inline static void check_remove_bel() {
    char *e;
    
    if (remove_bel != -1)
        return;
    
    if ((e = getenv("SB_REMOVE_BEL")))
        remove_bel = (strcasecmp(e, "no") == 0 || strcasecmp(e, "n") == 0 || strcasecmp(e, "off") == 0 || strcmp(e, "0") == 0) ? 0 : 1;
    else
        remove_bel = 1;
}

#define CACHE_SIZE 30
static int cache[CACHE_SIZE];
static int cache_init = 0;

static void cache_put(int fd, int v) {
    if (fd >= CACHE_SIZE)
        return;
    
    if (!cache_init) {
        int i;
        for (i = 0; i < CACHE_SIZE; i++)
            cache[i] = -1;
        cache_init = 1;
    }

    cache[fd] = v;
}

static int cache_get(int fd) {
    if (!cache_init)
        return -1;

    if (fd >= CACHE_SIZE)
        return -1;
    
    return cache[fd];
}

static int is_console(int fd) {
    int r;
    
    if ((r = cache_get(fd)) != -1)
        return r;

    r = isatty(fd) ? 1 : 0;

    if (r) {
        char *p = ptsname(fd);

#ifdef DEBUG
        debug("ptsname(%i) is '%s'\n", fd, p);
#endif
        
        // No filtering when master pty
        if (p)
            r = 0;
    }
    
    cache_put(fd, r);
    return r;
}

static char* get_appname() {
    static char* appname = NULL;
    FILE* f;
    char fn[PATH_MAX];
    
    if (appname)
        return appname;

    snprintf(fn, sizeof(fn), "/proc/%i/cmdline", getpid());
    f = fopen(fn, "r");
    fgets(fn, sizeof(fn), f);
    fclose(f);

    return (appname = strdup(fn));
}

static void do_beep(void) {
    int _errno = errno;
    static char *e = NULL;

#ifdef DEBUG
    static int c = 0;
    debug("[%u|%s] Beep #%i\n", getpid(), get_appname(), ++c);
#endif

    if (!e) {
        char *p;

        // I know, the memory of e is never freed, but who cares? It
        // is allocated once at most.
        
        if ((p = getenv("SB_RUN")))
            asprintf(&e, "%s \"%s\"", p, get_appname());
        else
            e = "sb-beep";
    }

    system(e);    
    errno = _errno;
}

// close
// fclose

int close(int FILEDES) {
    static int (*func) (int) = NULL;

    if (!func)
        func = (int (*) (int)) dlsym(REAL_LIBC, "close");

    cache_put(FILEDES, -1);
    
    return func(FILEDES);
}

int fclose(FILE *STREAM) {
    static int (*func) (FILE*) = NULL;

    if (!func)
        func = (int (*) (FILE*)) dlsym(REAL_LIBC, "fclose");

    cache_put(fileno(STREAM), -1);
    
    return func(STREAM);
}

// write
// fwrite

// fputs
// puts

// fprintf
// printf

// putchar
// putc (_IO_putc !!!)
// fputc

static int magic_seq = 0;

inline static void magic_detect_char(char c) {
    if (magic_seq == 0 && c == ESC)
        magic_seq++;
    else if (magic_seq == 1 && c == ']')
        magic_seq++;
    else if (magic_seq == 2 && isdigit(c))
        magic_seq++;
    else if (c == BEL)
        magic_seq = 0;
    else if (magic_seq != 0 && magic_seq != 3)
        magic_seq = 0;
}

inline static void magic_detect_string(const char *p, int l) {
    for (; l; p++, l--)
        magic_detect_char(*p);
}

ssize_t write(int FILEDES, const void *BUFFER, size_t SIZE) {
    static ssize_t (*func) (int, const void*, size_t) = NULL;

    if (!func)
        func = (ssize_t (*) (int, const void*, size_t)) dlsym (REAL_LIBC, "write");

    check_remove_bel();
    
    if (is_console(FILEDES) && SIZE != 0 && BUFFER != NULL) {
        const void *n, *l = BUFFER;
        ssize_t sum = 0;
        size_t len = SIZE;

        while ((n = memchr(l, BEL, len))) {
            if (l != n) {
                ssize_t r;
                size_t c = n-l;

                magic_detect_string(l, c);
                
                if ((r = (*func)(FILEDES, l, c)) != c)
                    return r >= 0 ? sum+r : r;
                
                sum += c;
                len -= c;
            }

            if (!magic_seq)
                do_beep();

            if (!remove_bel || magic_seq) {
                ssize_t r;
                char bel = BEL;

                magic_detect_char(bel);

                if ((r = (*func)(FILEDES, &bel, 1)) != 1)
                    return r >= 0 ? sum+r : r;
            } else if (remove_bel)
                magic_detect_char(BEL);
            
            sum++;
            len--;

            l = n+1;
        }

        if (len) {
            ssize_t r;

            magic_detect_string(l, len);
            
            if ((r = (*func)(FILEDES, l, len)) != len)
                return r >= 0 ? sum+r : r;

            sum += len;
        }

        return sum;        
    } else
        return (*func)(FILEDES, BUFFER, SIZE);
}

size_t fwrite(const void *DATA, size_t SIZE, size_t COUNT, FILE *STREAM) {
    static ssize_t (*func) (const void*, size_t, size_t, FILE*) = NULL;

    if (!func)
        func = (ssize_t (*) (const void*, size_t, size_t, FILE*)) dlsym (REAL_LIBC, "fwrite");

    check_remove_bel();
    
    if (is_console(fileno(STREAM)) && SIZE != 0 && COUNT != 0 && DATA != NULL) {
        const void *n, *l = DATA;
        size_t sum = 0, len = SIZE*COUNT;
        int fl = 0;

        while ((n = memchr(l, BEL, len))) {
            if (l != n) {
                size_t c = n-l;

                magic_detect_string(l, c);
                
                if ((*func)(l, c, 1, STREAM) != 1)
                    return sum/SIZE;
                
                sum += c;
                len -= c;
            }

            if (!magic_seq)
                do_beep();

            if (!remove_bel || magic_seq) {
                char bel = BEL;

                magic_detect_char(bel);

                if ((*func)(&bel, 1, 1, STREAM) != 1)
                    return sum/SIZE;
            } else if (remove_bel)
                magic_detect_char(BEL);
            
            sum++;
            len--;

            l = n+1;
        }

        if (len) {
            magic_detect_string(l, len);
            
            if ((*func)(l, len, 1, STREAM) != 1)
                return sum/SIZE;

            sum += len;
        }

        if (fl)
            fflush(STREAM);

        return sum/SIZE;
    } else
        return (*func)(DATA, SIZE, COUNT, STREAM);
}


int fputs(const char *S, FILE *STREAM) {
    int l = strlen(S);
    int r;
    
    r = fwrite(S, l, 1, STREAM);

    if (strchr(S, '\n'))
        fflush(STREAM);

    return r != 1 ? EOF : 1;
}

int puts(const char *S) {
    int r, t = 0;

    if ((r = fputs(S, stdout)) == EOF)
        return EOF;
    
    if ((t = fputc('\n', stdout)) == EOF)
        return EOF;

    return r+t;
}

int vfprintf(FILE *STREAM, const char *TEMPLATE, va_list AP) {
    int r;
    char *p;

    if ((r = vasprintf(&p, TEMPLATE, AP)) >= 0) {
        if (fputs(p, STREAM) == EOF)
            r = -1;

        free(p);
    }
    return r;
}

int fprintf(FILE *STREAM, const char *TEMPLATE, ...) {
    int r;
    va_list ap;

    va_start(ap, TEMPLATE);
    r = vfprintf(STREAM, TEMPLATE, ap);
    va_end(ap);
    return r;
}

int printf(const char *TEMPLATE, ...) {
    int r;
    va_list ap;

    va_start(ap, TEMPLATE);
    r = vfprintf(stdout, TEMPLATE, ap);
    va_end(ap);
    return r;
}

int fputc(int C, FILE *STREAM) {
    static int (*func) (int, FILE*) = NULL;

    if (!func)
        func = (int (*) (int, FILE*)) dlsym(REAL_LIBC, "fputc");

    check_remove_bel();
    
    if (is_console(fileno(STREAM)))
        if (C == BEL)
            if (!magic_seq) {
                do_beep();

                if (remove_bel) {
                    magic_detect_char(BEL);
                    return BEL;
                }
            }
    
    magic_detect_char(C);
    return func(C, STREAM);
}

int putchar(int C) {
    static int (*func) (int) = NULL;

    if (!func)
        func = (int (*) (int)) dlsym(REAL_LIBC, "putchar");

    check_remove_bel();
    
    if (is_console(fileno(stdout)))
        if (C == BEL)
            if (!magic_seq) {
                do_beep();

                if (remove_bel) {
                    magic_detect_char(BEL);
                    return BEL;
                }
            }

    magic_detect_char(C);
    return func(C);
}


int _IO_putc(int C, _IO_FILE *STREAM) {
    static int (*func) (int, FILE*) = NULL;

    if (!func)
        func = (int (*) (int, FILE*)) dlsym(REAL_LIBC, "_IO_putc");

    check_remove_bel();
    
    if (is_console(fileno(STREAM)))
        if (C == BEL)
            if (!magic_seq) {
                do_beep();

                if (remove_bel) {
                    magic_detect_char(BEL);
                    return BEL;
                }
            }

    magic_detect_char(C);
    return func(C, STREAM);
}

int beep(void) {
    do_beep();
    return 0;
}

int XBell(Display* display, int percent) {
    do_beep();
    return 0;
}

void gdk_beep (void) {
    do_beep();
}
