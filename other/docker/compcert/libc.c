#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

static int errno_value = 0;

int *__errno_location(void) { return &errno_value; }

char *strrchr(const char *s, int c) { return NULL; }

uint16_t htons(uint16_t hostshort)
{
    return ((hostshort << 8) & 0xff00) | ((hostshort >> 8) & 0x00ff);
}

uint16_t ntohs(uint16_t netshort) { return htons(netshort); }

void *memcpy(void *dst, const void *src, size_t n)
{
    char *d = dst;
    const char *s = src;
    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }
    return dst;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    for (; n--; p1++, p2++) {
        const unsigned char u1 = *p1;
        const unsigned char u2 = *p2;
        if (u1 != u2) {
            return u1 - u2;
        }
    }
    return 0;
}

void *memset(void *s, int c, size_t n)
{
    char *ptr = s;
    for (size_t i = 0; i < n; ++i) {
        ptr[i] = c;
    }
    return s;
}

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len] != '\0') {
        ++len;
    }
    return len;
}

void *calloc(size_t nmemb, size_t size)
{
    void *ptr = malloc(nmemb * size);
    if (ptr) {
        memset(ptr, 0, nmemb * size);
    }
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        return malloc(size);
    }
    return NULL;
}

time_t time(time_t *tloc) { return 1706221190; }

int clock_gettime(clockid_t clockid, struct timespec *tp)
{
    tp->tv_sec = 1706221190;
    tp->tv_nsec = 0;
    return 0;
}

int socket(int domain, int type, int protocol) { return 5; }

int bind(int socket, const struct sockaddr *address, socklen_t address_len) { return 0; }

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    memcpy(dst, "127.0.0.1", sizeof("127.0.0.1"));
    return dst;
}

int setsockopt(
    int socket, int level, int option_name, const void *option_value, socklen_t option_len)
{
    return 0;
}

int getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len)
{
    return 0;
}

int fcntl(int fildes, int cmd, ...) { return 0; }

int close(int fildes) { return 0; }
