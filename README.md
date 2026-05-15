*This project has been created as part of the 42 curriculum by acombier.*

# Codexion

> *Master the race for resources before the deadline masters you.*

---

## Description

**Codexion** is a concurrent simulation written in C. It is a variant of
Dijkstra's *Dining Philosophers* problem, adapted to a software-engineering
metaphor:

- A circular co-working hub hosts **N coders** (POSIX threads).
- On the table sit **N USB dongles** (shared resources protected by mutexes).
- Each coder needs **two adjacent dongles** (its left and its right) at the
  same time to **compile**. Compiling resets its personal *burnout timer*.
- Between compiles, the coder **debugs**, then **refactors**, then tries to
  compile again.
- A coder who fails to start a new compile within `time_to_burnout` ms
  **burns out** and the simulation stops.

On top of the classical formulation, the project adds three twists:

1. **Cooldown** — a released dongle stays unavailable for
   `dongle_cooldown` ms before it can be picked up again.
2. **Scheduling policy** — when several coders want the same dongle, ties
   are broken by either:
   - **FIFO**: serve the coder who waited the longest (anti-starvation
     ticket = `compiles_done * N + coder_id`),
   - **EDF** *(Earliest Deadline First)*: serve the coder whose burnout
     deadline is closest (`last_compile_start_ms + time_to_burnout`).
3. **Priority queue** — the per-dongle waiting list is a **binary min-heap**
   we wrote ourselves (no `qsort`, no std).

A dedicated **monitor thread** polls the coders' last-compile timestamps
every 500 µs and reports a burnout within the 10 ms precision required by
the subject.

---

## Instructions

### Build

```sh
make            # builds ./codexion
make clean      # removes obj/
make fclean     # removes obj/ and ./codexion
make re         # full rebuild
```

Compiles with `-Wall -Wextra -Werror -pthread`. Sources are listed
explicitly in the `Makefile` (no `$(wildcard ...)`, as required by the 42
Norm).

### Usage

```sh
./codexion number_of_coders \
           time_to_burnout \
           time_to_compile \
           time_to_debug \
           time_to_refactor \
           number_of_compiles_required \
           dongle_cooldown \
           scheduler
```

| Argument | Type | Meaning |
|---|---|---|
| `number_of_coders` | int > 0 | Number of coders **and** dongles |
| `time_to_burnout` | ms | Max delay between two compile starts before burnout |
| `time_to_compile` | ms | Duration of one compile (dongles held) |
| `time_to_debug` | ms | Duration of debug (dongles released) |
| `time_to_refactor` | ms | Duration of refactor (dongles released) |
| `number_of_compiles_required` | int > 0 | Per coder; simulation ends when **all** reach it |
| `dongle_cooldown` | ms | Time a dongle is unavailable after release |
| `scheduler` | string | `fifo` or `edf` |

All arguments are mandatory. Invalid input (missing args, non-numeric,
negative, overflow > `INT_MAX`, scheduler ≠ `fifo`/`edf`) prints an error
to `stderr` and exits with code `1`.

### Log format

Every state transition is printed as `timestamp_ms coder_id message`:

```
0 1 has taken a dongle
1 1 has taken a dongle
1 1 is compiling
201 1 is debugging
401 1 is refactoring
...
1505 4 burned out
```

Lines are serialized through a global `log_mutex` — no two messages ever
interleave.

### Examples

```sh
# 5 coders, no burnout expected, FIFO
./codexion 5 800 200 200 200 5 0 fifo

# Same load but EDF scheduling
./codexion 5 800 200 200 200 7 0 edf

# Burnout demonstration: cycle (200+100+100) > burnout (310)
./codexion 4 310 200 100 100 5 0 fifo

# Single coder: only 1 dongle on the table -> burnout is the expected
# behaviour (subject Chapter VI: "If there is only one coder, there should
# be only one dongle on the table.")
./codexion 1 800 200 200 200 5 0 fifo
```

### Memory & race testing

```
valgrind --tool=helgrind ./codexion 5 800 200 200 200 5 0 fifo
valgrind --tool=drd      ./codexion 5 800 200 200 200 5 0 fifo
```

## Project structure

```
inc/codexion.h              Single public header (all types + prototypes)
src/main.c                  Entry point: parse -> init -> launch -> join -> destroy
src/parse_args.c            Safe atoi (no '-', no '+', no overflow) + scheduler enum
src/sim.c                   sim_init / sim_destroy + per-stage init helpers
src/sim_run.c               sim_launch / sim_join / sim_emergency_stop
src/sim_time.c              now_ms / elapsed_ms / precise_sleep_ms / ms_to_timespec
src/log.c                   Generic log_event + 4 normal-state wrappers
src/log_burnout.c           Special-case burnout log (always prints, then sets stop)
src/coder.c                 coder_routine + take/release/cycle helpers
src/dongle.c                dongle_init / destroy / request / release
src/dongle_utils.c          Priority-key computation, can_take check, wait loop
src/monitor.c               Burnout detector + completion detector
src/pqueue.c                Min-heap: init / push / peek / pop
src/pqueue_utils.c          Heap helpers: grow / sift_up / sift_down / remove
```

---

## Blocking cases handled

### 1. Deadlock prevention (Coffman conditions)

The classical Dining Philosophers deadlock happens when every coder grabs
its left dongle simultaneously and then waits forever for its right one.
The four Coffman conditions are: **mutual exclusion**, **hold-and-wait**,
**no preemption**, **circular wait**. We break the *circular wait*
condition by introducing an **asymmetric acquisition order**
(see `src/coder.c:24-36`):

- **even-id** coders grab **left first**, then right;
- **odd-id** coders grab **right first**, then left.

With this rule, two neighbours can no longer end up waiting on each other
in a cycle. Release follows reverse acquisition order (LIFO,
`coder.c:45-57`) to minimise priority inversions in the per-dongle queue.

### 2. Starvation prevention

Every dongle owns its **own min-heap of waiters** (`t_pqueue`,
`pqueue.c`). When a coder calls `dongle_request`, the heap is sorted by a
key computed from the current scheduling policy (`dongle_utils.c:37-55`):

- **FIFO**: `key = compiles_done * N + coder_id`. A coder that has
  compiled fewer times **always** outranks a busier one, regardless of
  arrival order. This is the anti-starvation guarantee.
- **EDF**: `key = last_compile_start_ms + time_to_burnout`. The coder
  whose burnout deadline is the closest is served first. Liveness still
  holds as long as the parameters are feasible.

A coder can take a dongle only if it is at the **top of the heap**
(`dongle_utils.c:74-87`). The heap is updated under the dongle's mutex,
so the ordering is globally consistent for that resource.

### 3. Cooldown handling

After release, a dongle stores its `released_at_ms` timestamp
(`dongle.c:111-118`). The `dongle_can_take` predicate refuses the dongle
while `now_ms() - released_at_ms < dongle_cooldown`. The initial value
`released_at_ms = 0` means "never released yet" and is treated as
*no cooldown* (`dongle_utils.c:82`).

The waiting loop wakes up periodically (`usleep(200)` µs poll) so it can
re-check the cooldown without depending on a broadcast — useful when the
dongle is *free but not yet ready*.

### 4. Precise burnout detection (< 10 ms)

The **monitor thread** (`monitor.c:127-148`) sleeps for `500 µs` between
two scans. Each scan reads every coder's `last_compile_start_ms` and
`compiles_done` under `coder_state_mutex` (`monitor.c:52-72`). If a
running coder (still below the required number of compiles) shows
`now - last_compile_start > time_to_burnout`, the monitor calls
`monitor_trigger_burnout`, which:

1. logs `"<ts> <id> burned out"` and sets `sim->stop = 1`
   **atomically under `log_mutex`** (see Synchronization, §B below);
2. broadcasts on every dongle's condvar **and** sets
   `dongles[i].stop_requested = 1`, waking any coder waiting on a dongle.

The 500 µs cadence is two orders of magnitude below the 10 ms tolerance
required by the subject.

### 5. Log serialization

All state-change logs go through `log_event` (`log.c:28-44`), which holds
`log_mutex` while printing the line. Two messages can therefore **never**
interleave on a single line. `log_event` also rechecks `sim->stop` (under
`stop_mutex`) before printing, which guarantees that **no state log is
emitted after `"burned out"`**. The burnout line itself uses a slightly
different routine (`log_burned_out`, `log_burnout.c:34-51`) that bypasses
the `if (stop) return` check, since it has to print even when stop is
about to be raised.

---

## Thread synchronization mechanisms

The implementation uses four kinds of synchronization objects. The lock
hierarchy is **`log_mutex → stop_mutex → coder_state_mutex → dongle.mutex`**;
every code path acquires locks in (a subset of) that order to avoid lock
inversion.

### A. Per-`t_sim` mutexes (3)

| Mutex | Protects | Why |
|---|---|---|
| `sim->log_mutex` | `printf` + the "should I print?" decision | Without it, two coders could write half-lines to stdout interleaved, breaking the parser. It also serializes the read of `sim->stop` together with the print so that no state line is emitted after `burned out`. |
| `sim->stop_mutex` | `sim->stop` (int flag) | The flag is written by the monitor and read by every coder. Without a mutex, helgrind/drd correctly flag a data race; a torn read is also theoretically possible. |
| `sim->coder_state_mutex` | `last_compile_start_ms` and `compiles_done` of **all** coders | Both are 64-bit / 32-bit values touched by the coder thread *and* the monitor. On non-atomic-store architectures a torn read could give the monitor a stale "since last compile" value and trigger a spurious burnout. |

### B. Atomic *log + state* update under `log_mutex`

`log_burned_out` does two things **under the same lock**
(`log_burnout.c:34-51`):

```c
lock(log_mutex);
lock(stop_mutex);
if (sim->stop) { unlock; unlock; return; }   // someone burned out first
sim->stop = 1;
unlock(stop_mutex);
printf("%lld %d burned out\n", ts, coder_id);
unlock(log_mutex);
```

This pattern guarantees three things at once:

1. **At most one coder is reported as burned out** — the first one to
   enter the critical section sets `stop`, the others see `stop == 1`
   and return without printing.
2. **No state log can sneak in between `stop = 1` and the burnout line**,
   because every `log_event` also takes `log_mutex` first and then reads
   `stop`.
3. **No state log can appear after `burned out`** for the same reason.

### C. Per-`t_dongle` mutex + condvar + flag

```c
typedef struct s_dongle {
    int              id;
    pthread_mutex_t  mutex;
    int              taken;
    int              stop_requested;   /* set by the monitor */
    long long        released_at_ms;
    t_pqueue        *waiters;          /* min-heap of coder_ids */
    pthread_cond_t   cond;
} t_dongle;
```

`mutex` protects every other field. `cond` is initialized and
*broadcast* upon every release and upon shutdown — coders **do not**
currently call `pthread_cond_wait` on it; the waiting loop is a short
polling loop (`usleep(200)` µs) that re-checks `taken`, cooldown, and the
head of the waiters' heap on every tick (`dongle_utils.c:114-123`). This
keeps the wake-up path simple while still respecting the 10 ms burnout
deadline. The `stop_requested` flag was added on top of the condvar so
that the wait loop can detect a shutdown without taking `stop_mutex`
(which would violate the lock order, since we are already holding
`dongle->mutex`).

### D. The per-dongle priority queue (custom heap)

`pqueue.c` / `pqueue_utils.c` implement a min-heap of `t_pqnode`:

```c
typedef struct s_pqnode {
    int        coder_id;
    long long  key;       /* FIFO ticket or EDF deadline */
    long long  tiebreak;  /* = coder_id, deterministic */
} t_pqnode;
```

The heap is **scheduler-agnostic**: the caller computes the right key
before calling `pq_push`. `dongle_compute_key` (`dongle_utils.c:37-55`)
is the single place where the FIFO/EDF policy is decided, which keeps
the heap reusable and easy to test.

### E. Coordination patterns

**Coders ↔ dongle.** `dongle_request` (`dongle.c:83-97`) inserts the
coder in the heap, polls `dongle_can_take` until it returns true, then
flips `taken = 1` and releases the mutex. Logging is done *outside* the
critical section to respect the lock hierarchy (`log_mutex` must be
above `dongle.mutex`).

**Coders ↔ monitor.** They communicate via two shared fields, each
protected by a dedicated mutex:

- the monitor reads `last_compile_start_ms` and `compiles_done` (set by
  coders under `coder_state_mutex`);
- the coders read `sim->stop` (set by the monitor under `stop_mutex`)
  and `dongles[i].stop_requested` (set by the monitor under
  `dongle.mutex` — same mutex the wait loop is using anyway).

**Why `last_compile_start_ms` is updated *before* `log_compiling`**
(`coder.c:78-82`): if the order were reversed, the monitor could grab
`coder_state_mutex`, read a stale timestamp, compute a too-large
`since_last_compile`, and report a fake burnout *between* the print and
the assignment.

**Why the dongles are released *before* debug and refactor**
(`coder.c:90-97`): holding the resource during debug/refactor would
serialise unrelated coders for no reason. The subject is explicit
("they put both dongles back on the table and start debugging").

**Why `precise_sleep_ms` reads `sim->stop` under `stop_mutex`**
(`sim_time.c:57-71`): the previous version polled `sim->stop` without a
lock, which helgrind flagged as a data race against the monitor's write.
Since `precise_sleep_ms` holds no other lock when called, taking
`stop_mutex` does not violate the global lock order.

---

## Resources

### Concurrency & scheduling

- **Dijkstra, E. W.** *Cooperating Sequential Processes*, 1965 — the
  original Dining Philosophers formulation.
- **Coffman, E. G., Elphick, M. J., Shoshani, A.** *System Deadlocks*,
  ACM Computing Surveys, 1971 — the four necessary conditions for
  deadlock (mutual exclusion, hold-and-wait, no preemption, circular
  wait).
- **Liu, C. L. & Layland, J. W.** *Scheduling Algorithms for
  Multiprogramming in a Hard-Real-Time Environment*, JACM 1973 — formal
  proof of EDF optimality on a single processor.
- `man pthreads(7)`, `man pthread_mutex_lock(3)`,
  `man pthread_cond_timedwait(3)`, `man gettimeofday(2)`.
- *Programming with POSIX Threads*, David R. Butenhof — reference book on
  mutex/condvar patterns, especially Chapter 3 on the predicate loop
  idiom (`while (!cond) wait;`).

### Data structures

- **CLRS**, *Introduction to Algorithms*, 3rd ed., Chapter 6 — binary
  heap, sift_up / sift_down (`pqueue_utils.c`).

### Tools

- `valgrind`'s **memcheck**, **helgrind**, and **drd** were used to
  audit memory and concurrency (see *Memory & race testing* above).
- `gdb` and `cgdb` for stepwise debugging of cycle ordering.

### Use of AI

A conversational AI assistant (Anthropic's Claude) was used during this
project for the following, bounded scope:

| Task | AI contribution | Human contribution |
|---|---|---|
| Subject reading & breakdown into phases | Drafted the phase plan in `TODO.txt`. | Reviewed, edited, accepted phase boundaries. |
| Algorithm design (heap, EDF/FIFO keys, lock order) | Suggested candidate designs and trade-offs. | All decisions (option B for deadlock, lock hierarchy, polling vs. condwait) were taken by hand and justified above. |
| Code review | Pointed out two real bugs: the `sim->stop` read without `stop_mutex` in `precise_sleep_ms`, and the `stop_requested` flag needed to avoid taking `stop_mutex` from inside `dongle.mutex`. | The fixes were re-derived, written, and re-read by hand. |
| Test design | Generated the invalid-input matrix (negatives, `+`, overflow, mixed case) and the stress recipes (100 coders, valgrind under load). | Tests were run, results were interpreted, and failures (Test 11.3.1) were investigated manually. |
| Documentation | Drafted this README and the comments. | Read end-to-end and adapted to match the code that actually shipped (e.g. correcting the comments that mentioned `cond_timedwait` while the wait loop in `dongle_utils.c:114-123` is in fact a `usleep` poll). |

No AI-generated code was committed without being read, understood, and
re-typed by hand. The author can explain every mutex, every cond, the
heap, the FIFO/EDF policies, and the lock hierarchy without referring to
the AI transcript.

---

## Known limitations

- **Edge case `cooldown=200, scheduler=edf, 5 coders`**
  (`./codexion 5 1000 100 100 100 10 200 edf`): under a long cooldown,
  the asymmetric pair/odd acquisition order can lock a coder on its
  second dongle while its first dongle is already free. EDF only sorts
  the queue of a *single* dongle, so it cannot reorder the
  *acquisition sequence*. Workarounds: lower the cooldown, or refactor
  the acquisition under a global "table" arbiter (Coffman's
  *no preemption* alternative). The current code prefers determinism and
  clarity over coverage of this corner case.
- The `pthread_cond_t` inside `t_dongle` is currently broadcast but
  never `wait`-ed on (the wait loop polls). This was a conscious
  simplification to keep the wake-up path under a single lock order; it
  could be tightened to a `pthread_cond_timedwait` to reduce idle CPU
  use at very low loads.
