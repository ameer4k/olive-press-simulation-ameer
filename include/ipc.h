#ifndef OLIVE_IPC_H
#define OLIVE_IPC_H

#include <signal.h>
#include <stddef.h>
#include <sys/types.h>

#include "config.h"

/*
 * Control signals, supervisor -> child.
 *
 *   SIG_PAUSE   stop taking new work, finish nothing, report PAUSED
 *   SIG_RESUME  go back to whatever state was active before the pause
 *   SIG_FAULT   force the machine into ERROR (manual fault injection,
 *               or the supervisor reacting to a QC alarm)
 *   SIG_CLEAR   leave ERROR and continue
 *   SIGTERM     clean shutdown: close pipes, send EV_EXIT, exit(0)
 *
 * SIGRTMIN is not a compile time constant, so these must stay macros.
 */
#define SIG_PAUSE   SIGUSR1
#define SIG_RESUME  SIGUSR2
#define SIG_FAULT   (SIGRTMIN + 0)
#define SIG_CLEAR   (SIGRTMIN + 1)

/*
 * Everything a child needs after fork(). Unused descriptors are -1 and
 * every descriptor not listed here has already been closed by the parent
 * side of the fork.
 *
 *   in_fd      batch pipe from the previous stage (-1 for reception)
 *   out_fd     batch pipe to the next stage (-1 for separation)
 *   telem_fd   MachineEvent stream to the supervisor (all children share it)
 *   result_fd  ProductionResult stream (separation only)
 *   alarm_fd   Alarm stream (quality control only)
 *   qc_in_fd   MachineEvent copies forwarded by the supervisor (QC only)
 */
typedef struct {
    int           id;
    const Config *cfg;
    int           in_fd;
    int           out_fd;
    int           telem_fd;
    int           result_fd;
    int           alarm_fd;
    int           qc_in_fd;
} StageCtx;

/* Entry point of every child. Returns the exit status. */
typedef int (*stage_fn)(const StageCtx *ctx);

int reception_run(const StageCtx *ctx);
int cleaning_run(const StageCtx *ctx);
int washing_run(const StageCtx *ctx);
int crushing_run(const StageCtx *ctx);
int malaxation_run(const StageCtx *ctx);
int separation_run(const StageCtx *ctx);
int quality_control_run(const StageCtx *ctx);

/*
 * Loop until exactly len bytes moved, retrying on EINTR and short transfers.
 * read_full returns len on success, 0 on clean EOF before any byte, -1 on
 * error (including EOF in the middle of a message, errno = EPIPE).
 * write_full returns len or -1.
 */
ssize_t read_full(int fd, void *buf, size_t len);
ssize_t write_full(int fd, const void *buf, size_t len);

int set_nonblocking(int fd);
int set_cloexec(int fd);

#endif
