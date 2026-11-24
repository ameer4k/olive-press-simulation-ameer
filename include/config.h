#ifndef OLIVE_CONFIG_H
#define OLIVE_CONFIG_H

#include <stddef.h>

typedef struct {
    double min;
    double max;
} Range;

typedef struct {
    Range  duration;        /* seconds per batch */
    double heat_rate;       /* deg C per second while operating */
    double cool_rate;       /* 1/s, Newton cooling toward ambient */
    double max_temp;        /* QC limit, deg C */
    double base_pressure;   /* bar while operating */
    double max_pressure;    /* QC limit, bar */
    double debris_removal;  /* share of remaining impurity removed here */
} MachineCfg;

typedef struct {
    unsigned   seed;
    int        num_batches;
    int        queue_capacity;
    Range      batch_mass;      /* kg */
    Range      impurity;        /* fraction */
    Range      oil_potential;   /* fraction */
    Range      water_fraction;  /* vegetable water share of the paste */
    double     extraction_eff;  /* share of oil the decanter recovers */
    double     ambient_temp;    /* deg C */
    double     tick;            /* seconds between telemetry samples */
    int        auto_recover;    /* supervisor clears overtemp faults itself */
    double     recover_ratio;   /* clear once temp < max_temp * ratio */
    MachineCfg m[6];
} Config;

void config_defaults(Config *cfg);

/*
 * Load "key = value" lines on top of the defaults. Unknown keys are an
 * error so typos do not go unnoticed. Returns 0 on success, -1 with a
 * message in err otherwise.
 */
int config_load(const char *path, Config *cfg, char *err, size_t errlen);

int config_validate(const Config *cfg, char *err, size_t errlen);

#endif
