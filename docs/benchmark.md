# OpenMP benchmark

`benchmark/openmp_benchmark.c`, built as `./omp_bench`.

## What is being measured

Each "sample" is one olive lot sent to the lab. For every lot the program
integrates a malaxation model with 4th order Runge-Kutta:

* oil recovery `dY/dt = k(T) (Ymax - Y)` with an Arrhenius rate
  `k(T) = A exp(-Ea / R T)`,
* phenolic quality `dQ/dt = -kq Q max(0, T - 27)`, i.e. quality only drops
  above the cold extraction limit,
* paste temperature `T(t) = T_amb + T_rise (1 - e^(-t/tau))`, the same
  heat-up shape the simulated machines follow.

Every lot gets its own oil potential, hall temperature, heating and duration
from a hash of `(seed, index)`. Lots share nothing, and each one is a long
chain of `exp()` calls, so the loop is CPU bound and has no data races. The
same function runs in both versions:

```c
/* sequential */
for (i = 0; i < n; i++) { out[i] = analyse_sample(seed, i, steps); sum += out[i].yield; }

/* OpenMP */
#pragma omp parallel for num_threads(t) schedule(static) reduction(+:sum)
for (i = 0; i < n; i++) { out[i] = analyse_sample(seed, i, steps); sum += out[i].yield; }
```

`schedule(static)` fits because every sample costs the same number of steps.
`reduction(+:sum)` gives each thread a private partial sum, which avoids a
race on the total.

## Correctness check

* Per-sample results are compared with `memcmp`: they must be bit-identical,
  because each sample's inputs depend only on its index, never on which thread
  ran it or in what order.
* The total yield is a reduction, so the floating point addition order
  changes with the thread count. It is compared with a relative tolerance of
  `1e-12`.

The program exits non zero if either check fails.

## Timing method

`omp_get_wtime()`, one warm-up run, then the median of `-r` repeats (5 by
default) for both the sequential and the parallel version.

## Results

Measured on a 2 core Linux VM (gcc 13.3, `-O2`). **Re-run this on the demo
machine** and paste your own numbers. The shape will be the same, but the
absolute times and the best thread count depend on the CPU.

Large workload, `./omp_bench --sweep -t 4` (4000 samples x 4000 steps,
128 M `exp()` calls per run):

| threads | sequential [s] | OpenMP [s] | speedup | efficiency | check |
|--------:|---------------:|-----------:|--------:|-----------:|-------|
| 1       | 1.1327         | 1.0796     | 1.05x   | 105 %      | match |
| 2       | 1.1327         | 0.5897     | 1.92x   | 96 %       | match |
| 4       | 1.1327         | 0.5623     | 2.01x   | 50 %       | match |

Tiny workload, `./omp_bench -n 2 -s 10 -r 101 --sweep -t 2`:

| threads | sequential [s] | OpenMP [s] | speedup | check |
|--------:|---------------:|-----------:|--------:|-------|
| 1       | 0.000001       | 0.000002   | 0.88x   | match |
| 2       | 0.000001       | 0.000002   | 0.73x   | match |

## Reading the numbers

* With 2 cores, 2 threads gives 1.92x, close to the ideal 2x. The work is
  independent and the threads touch separate parts of the output array, so
  there is almost nothing to lose.
* 4 threads on 2 cores does not help: the extra threads just time-share the
  same cores. Efficiency drops to about 50 %.
* 1 thread under OpenMP is about the same as plain sequential. The few
  percent either way is noise plus the outlined loop being optimised a bit
  differently.
* On a tiny job OpenMP is **slower** (0.73x). Waking the thread team and
  joining it costs a few microseconds, more than the work itself. This is the
  "check if you get better or worse results" part of the spec: OpenMP pays
  off only when each thread gets enough work to cover that fixed cost.

## Where this fits in the project

The simulator itself is process based: seven children talking over pipes,
with most of their time spent sleeping out batch durations. There is no hot
loop there for OpenMP to speed up, and adding threads inside a forked child
would only complicate signal handling. So OpenMP is applied to the part that
really is number crunching, the per-lot quality and yield analysis, and
measured on its own.
