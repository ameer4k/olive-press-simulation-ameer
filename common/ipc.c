#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "ipc.h"

ssize_t read_full(int fd, void *buf, size_t len)
{
    char  *p = buf;
    size_t done = 0;

    while (done < len) {
        ssize_t n = read(fd, p + done, len - done);
        if (n > 0) {
            done += (size_t)n;
        } else if (n == 0) {
            if (done == 0)
                return 0;
            errno = EPIPE;      /* writer vanished mid message */
            return -1;
        } else if (errno != EINTR) {
            return -1;
        }
    }
    return (ssize_t)done;
}

ssize_t write_full(int fd, const void *buf, size_t len)
{
    const char *p = buf;
    size_t      done = 0;

    while (done < len) {
        ssize_t n = write(fd, p + done, len - done);
        if (n >= 0)
            done += (size_t)n;
        else if (errno != EINTR)
            return -1;
    }
    return (ssize_t)done;
}

int set_nonblocking(int fd)
{
    int fl = fcntl(fd, F_GETFL);
    if (fl < 0)
        return -1;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

int set_cloexec(int fd)
{
    int fl = fcntl(fd, F_GETFD);
    if (fl < 0)
        return -1;
    return fcntl(fd, F_SETFD, fl | FD_CLOEXEC);
}
