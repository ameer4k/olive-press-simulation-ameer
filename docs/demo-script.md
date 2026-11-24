# Demo script

About 8 minutes. Build first with `make clean && make`, and make sure
`make test` passes on the demo machine.

## 1. Normal run (2 min)

```sh
./olive_press config/default.cfg
```

1. Window opens, all six boxes gray (IDLE), status READY. Nothing has been
   forked yet: `ps --forest -C olive_press` in a second terminal shows one
   process.
2. Click **Start**. Show `ps --forest` again: one parent, seven children, same
   PIDs as on the boxes.
3. Point at:
   * green boxes and the olive circles moving through them,
   * circles on the arrows: batches sitting in a pipe, waiting for a slower
     stage (malaxation is the bottleneck),
   * temperatures rising while a machine works and falling while it is idle,
   * Production panel filling in, and "mass balance err" staying at 0.
4. Let reception finish: it turns blue (COMPLETED) and its PID shows
   "(exited)". The EOF then travels down the line.

## 2. Operator controls (2 min)

On a running line:

1. **Pause**: all six turn yellow, temperatures start dropping. Mention this
   is `SIGUSR1` to six PIDs.
2. **Resume**: back to green, nothing lost.
3. Click **Crushing**, then **Inject fault**: crushing goes red, reception goes
   yellow (held so no new olives enter), and the batches already past
   reception keep moving until they queue up in front of crushing.
4. **Clear fault**: crushing continues, reception is released.
5. **Stop**: everything stops within a moment; terminal shows each child
   exiting. `ps` shows no leftover processes.

## 3. Automatic fault handling (2 min)

```sh
./olive_press config/hot_day.cfg --autostart
```

Ambient 24 C and a hard working malaxer. After about 12 s malaxation passes the
27 C cold extraction limit:

* QC sends an Alarm, the supervisor faults the malaxer (red) and holds
  reception (yellow),
* the malaxer cools; once it is below 27 x 0.93 the supervisor clears it
  automatically and the line carries on.

The event log shows the alarm and the clear in red.

## 4. Crash handling (30 s)

With a line running:

```sh
kill -9 <pid of washing from the GUI>
```

Washing goes red with "process died", the supervisor stops the rest safely,
no zombies (`ps` shows nothing).

## 5. Benchmark (1 min)

```sh
./omp_bench --sweep
./omp_bench -n 2 -s 10 -r 101 --sweep
```

First: speedup close to the number of cores, results match. Second: OpenMP is
slower on a tiny job because starting the thread team costs more than the work.

## Questions to be ready for

* **Why does the GUI run in the parent?** The supervisor already holds every
  read end and every PID. Putting the window there means one process owns both
  the display and the control. A separate GUI process would need yet another
  channel to receive state, and could show something different from what the
  supervisor believes.
* **Why can't the GUI just read the children's variables?** After `fork()`
  each child has its own copy of memory. A temperature changed in crushing
  only exists in crushing's address space; the only way the parent learns it
  is through the telemetry pipe.
* **How does telemetry change the colours?** Child sends a `MachineEvent` with
  its `state` -> `supervisor_poll(0)` reads it in the 33 ms timer ->
  `MachineView.state` is updated -> `display()` picks the colour from that.
* **How do the control signals work?** `kill(pid, SIGUSR1)` etc. The child's
  handler only sets a `volatile sig_atomic_t` flag; its main loop checks the
  flags each tick. Handlers have no `SA_RESTART`, so the signal also wakes the
  child from `poll()`/`nanosleep()` right away.
* **Why real-time signals for fault/clear?** Standard signals do not queue.
  Two quick deliveries of the same standard signal can merge into one;
  real-time signals are queued.
* **Why is the GUI never blocked?** `poll()` with timeout 0 checks the pipes,
  and only a pipe reported readable is read. Each message is smaller than
  `PIPE_BUF`, so a readable pipe always holds a whole message.
* **What does OpenMP actually benchmark?** The lab-side yield/quality
  integration per olive lot, not the process pipeline itself. See
  `docs/benchmark.md`.
