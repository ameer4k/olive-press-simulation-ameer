#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config.h"

static const char *const machine_keys[NUM_MACHINES] = {
    "reception", "cleaning", "washing", "crushing", "malaxation", "separation"
};

void config_defaults(Config *c)
{
    memset(c, 0, sizeof *c);

    c->seed           = 2026;
    c->num_batches    = 10;
    c->queue_capacity = 4;
    c->batch_mass     = (Range){ 400.0, 600.0 };
    c->impurity       = (Range){ 0.02, 0.06 };
    c->oil_potential  = (Range){ 0.16, 0.22 };
    c->water_fraction = (Range){ 0.45, 0.55 };
    c->extraction_eff = 0.88;
    c->ambient_temp   = 22.0;
    c->tick           = 0.1;
    c->auto_recover   = 1;
    c->recover_ratio  = 0.97;

    /*                 duration     heat  cool  maxT  baseP maxP  debris */
    c->m[M_RECEPTION]  = (MachineCfg){ {0.6, 0.9}, 0.20, 0.10, 45.0, 1.0, 2.0, 0.0 };
    c->m[M_CLEANING]   = (MachineCfg){ {0.8, 1.2}, 0.30, 0.12, 45.0, 1.2, 2.5, 0.7 };
    c->m[M_WASHING]    = (MachineCfg){ {1.0, 1.4}, 0.15, 0.20, 40.0, 2.0, 4.0, 1.0 };
    c->m[M_CRUSHING]   = (MachineCfg){ {1.4, 2.0}, 1.60, 0.15, 38.0, 3.0, 6.0, 0.0 };
    c->m[M_MALAXATION] = (MachineCfg){ {2.4, 3.2}, 0.55, 0.20, 27.0, 1.0, 2.0, 0.0 };
    c->m[M_SEPARATION] = (MachineCfg){ {1.8, 2.6}, 0.90, 0.15, 35.0, 4.5, 8.0, 0.0 };
}

static char *trim(char *s)
{
    char *end;

    while (isspace((unsigned char)*s))
        s++;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        *--end = '\0';
    return s;
}

static int parse_double(const char *v, double *out)
{
    char *end;

    errno = 0;
    *out = strtod(v, &end);
    return (errno || end == v || *trim(end) != '\0') ? -1 : 0;
}

static double *machine_field(MachineCfg *m, const char *field)
{
    if (!strcmp(field, "duration_min"))   return &m->duration.min;
    if (!strcmp(field, "duration_max"))   return &m->duration.max;
    if (!strcmp(field, "heat_rate"))      return &m->heat_rate;
    if (!strcmp(field, "cool_rate"))      return &m->cool_rate;
    if (!strcmp(field, "max_temp"))       return &m->max_temp;
    if (!strcmp(field, "base_pressure"))  return &m->base_pressure;
    if (!strcmp(field, "max_pressure"))   return &m->max_pressure;
    if (!strcmp(field, "debris_removal")) return &m->debris_removal;
    return NULL;
}

static double *global_field(Config *c, const char *key)
{
    static const struct { const char *name; size_t off; } map[] = {
        { "batch_mass_min",     offsetof(Config, batch_mass.min) },
        { "batch_mass_max",     offsetof(Config, batch_mass.max) },
        { "impurity_min",       offsetof(Config, impurity.min) },
        { "impurity_max",       offsetof(Config, impurity.max) },
        { "oil_potential_min",  offsetof(Config, oil_potential.min) },
        { "oil_potential_max",  offsetof(Config, oil_potential.max) },
        { "water_fraction_min", offsetof(Config, water_fraction.min) },
        { "water_fraction_max", offsetof(Config, water_fraction.max) },
        { "extraction_eff",     offsetof(Config, extraction_eff) },
        { "ambient_temp",       offsetof(Config, ambient_temp) },
        { "tick",               offsetof(Config, tick) },
        { "recover_ratio",      offsetof(Config, recover_ratio) },
    };

    for (size_t i = 0; i < sizeof map / sizeof map[0]; i++)
        if (!strcmp(key, map[i].name))
            return (double *)((char *)c + map[i].off);
    return NULL;
}

static int set_key(Config *c, const char *key, const char *val)
{
    const char *dot = strchr(key, '.');
    double     *dst = NULL;
    double      d;

    if (parse_double(val, &d) < 0)
        return -1;

    if (dot) {
        for (int i = 0; i < NUM_MACHINES; i++) {
            size_t n = strlen(machine_keys[i]);
            if ((size_t)(dot - key) == n && !strncmp(key, machine_keys[i], n)) {
                dst = machine_field(&c->m[i], dot + 1);
                break;
            }
        }
    } else if (!strcmp(key, "seed")) {
        c->seed = (unsigned)d;
        return 0;
    } else if (!strcmp(key, "num_batches")) {
        c->num_batches = (int)d;
        return 0;
    } else if (!strcmp(key, "queue_capacity")) {
        c->queue_capacity = (int)d;
        return 0;
    } else if (!strcmp(key, "auto_recover")) {
        c->auto_recover = d != 0.0;
        return 0;
    } else {
        dst = global_field(c, key);
    }

    if (!dst)
        return -2;
    *dst = d;
    return 0;
}

int config_load(const char *path, Config *cfg, char *err, size_t errlen)
{
    FILE *fp = fopen(path, "r");
    char  line[256];
    int   lineno = 0;

    if (!fp) {
        snprintf(err, errlen, "%s: %s", path, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof line, fp)) {
        char *hash, *eq, *key, *val;
        int   rc;

        lineno++;
        if ((hash = strchr(line, '#')))
            *hash = '\0';
        key = trim(line);
        if (*key == '\0')
            continue;

        eq = strchr(key, '=');
        if (!eq) {
            snprintf(err, errlen, "%s:%d: expected key = value", path, lineno);
            fclose(fp);
            return -1;
        }
        *eq = '\0';
        val = trim(eq + 1);
        key = trim(key);

        rc = set_key(cfg, key, val);
        if (rc == -1) {
            snprintf(err, errlen, "%s:%d: bad number '%s' for %s", path, lineno, val, key);
            fclose(fp);
            return -1;
        }
        if (rc == -2) {
            snprintf(err, errlen, "%s:%d: unknown key '%s'", path, lineno, key);
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return config_validate(cfg, err, errlen);
}

static int bad_range(Range r, double lo, double hi)
{
    return r.min < lo || r.max > hi || r.min > r.max;
}

int config_validate(const Config *c, char *err, size_t errlen)
{
    if (c->num_batches < 1 || c->num_batches > 10000) {
        snprintf(err, errlen, "num_batches must be 1..10000");
        return -1;
    }
    if (bad_range(c->batch_mass, 1.0, 1e6)) {
        snprintf(err, errlen, "batch_mass range is invalid");
        return -1;
    }
    if (bad_range(c->impurity, 0.0, 0.5) || bad_range(c->oil_potential, 0.0, 0.5) ||
        bad_range(c->water_fraction, 0.0, 0.8)) {
        snprintf(err, errlen, "impurity / oil_potential / water_fraction range is invalid");
        return -1;
    }
    if (c->oil_potential.max + c->water_fraction.max > 1.0) {
        snprintf(err, errlen, "oil_potential_max + water_fraction_max must not exceed 1");
        return -1;
    }
    if (c->extraction_eff <= 0.0 || c->extraction_eff > 1.0) {
        snprintf(err, errlen, "extraction_eff must be in (0, 1]");
        return -1;
    }
    if (c->tick < 0.01 || c->tick > 1.0) {
        snprintf(err, errlen, "tick must be 0.01..1.0 seconds");
        return -1;
    }
    for (int i = 0; i < NUM_MACHINES; i++) {
        const MachineCfg *m = &c->m[i];
        if (bad_range(m->duration, 0.01, 600.0) || m->cool_rate < 0.0 || m->heat_rate < 0.0 ||
            m->max_temp <= c->ambient_temp || m->max_pressure <= 0.0 ||
            m->debris_removal < 0.0 || m->debris_removal > 1.0) {
            snprintf(err, errlen, "settings for %s are invalid", machine_keys[i]);
            return -1;
        }
    }
    return 0;
}
