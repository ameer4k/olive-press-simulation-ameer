#ifndef OLIVE_COMMON_H
#define OLIVE_COMMON_H

#include <sys/types.h>

/*
 * Shared data types for the olive press simulation.
 * Every message below is a fixed-size struct sent over a pipe in one write(),
 * all of them are well under PIPE_BUF so writes from several children never
 * interleave.
 *
 * Do not change field order or meaning without telling the whole team.
 */

enum {
    M_RECEPTION = 0,
    M_CLEANING,
    M_WASHING,
    M_CRUSHING,
    M_MALAXATION,
    M_SEPARATION,
    M_QC,
    NUM_PROCS
};

#define NUM_MACHINES 6          /* stages that handle batches (QC excluded) */
#define NUM_BATCH_PIPES (NUM_MACHINES - 1)

typedef enum {
    ST_IDLE = 0,
    ST_OPERATING,
    ST_PAUSED,
    ST_ERROR,
    ST_COMPLETED,
    NUM_STATES
} MachineState;

typedef enum {
    EV_TELEMETRY = 0,   /* periodic sample, sent every tick */
    EV_STATE_CHANGE,    /* state field changed since the last event */
    EV_BATCH_IN,        /* batch_id just entered this machine */
    EV_BATCH_OUT,       /* batch_id left this machine (written downstream) */
    EV_EXIT             /* last message before the child exits */
} EventType;

typedef enum {
    FAULT_NONE = 0,
    FAULT_OVERTEMP,
    FAULT_OVERPRESSURE,
    FAULT_TIMEOUT,
    FAULT_MANUAL,
    FAULT_CHILD_DIED
} FaultCode;

typedef struct {
    int    id;
    double original_mass;   /* kg, when it arrived at reception */
    double current_mass;    /* kg, after upstream losses */
    double impurity;        /* fraction 0..1 of leaves, twigs, soil */
    double oil_potential;   /* fraction 0..1 of oil in the clean olives */
    double quality;         /* 0..100, degrades with heat and delay */
    int    stage;           /* machine id that last touched it */
} Batch;

typedef struct {
    int    machine;
    pid_t  pid;
    int    batch_id;        /* -1 when the machine holds nothing */
    int    state;           /* MachineState */
    double temperature;     /* deg C */
    double pressure;        /* bar */
    double active_time;     /* seconds spent OPERATING since start */
    double progress;        /* 0..1 on the current batch */
    int    event;           /* EventType */
} MachineEvent;

typedef struct {
    int    machine;
    int    batch_id;
    int    fault;           /* FaultCode */
    double value;           /* measured value that broke the limit */
    double limit;
} Alarm;

typedef struct {
    int    batch_id;
    double original_mass;
    double oil;
    double water;
    double pomace;
    double losses;          /* removed upstream: debris, soil, wash losses */
} ProductionResult;

const char *machine_name(int id);
const char *state_name(int st);
const char *fault_name(int code);

#endif
