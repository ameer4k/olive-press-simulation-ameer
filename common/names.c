#include "common.h"

static const char *const machine_names[NUM_PROCS] = {
    "Reception", "Cleaning", "Washing",
    "Crushing", "Malaxation", "Separation", "Quality Control"
};

static const char *const state_names[NUM_STATES] = {
    "IDLE", "OPERATING", "PAUSED", "ERROR", "COMPLETED"
};

const char *machine_name(int id)
{
    return (id >= 0 && id < NUM_PROCS) ? machine_names[id] : "?";
}

const char *state_name(int st)
{
    return (st >= 0 && st < NUM_STATES) ? state_names[st] : "?";
}

const char *fault_name(int code)
{
    switch (code) {
    case FAULT_NONE:         return "none";
    case FAULT_OVERTEMP:     return "over temperature";
    case FAULT_OVERPRESSURE: return "over pressure";
    case FAULT_TIMEOUT:      return "processing timeout";
    case FAULT_MANUAL:       return "manual fault";
    case FAULT_CHILD_DIED:   return "process died";
    }
    return "unknown";
}
