# Rael – Phase 4 (Star8 + 5 Lanes, no GUI)
Star8 runs 8 worker threads (nodes). `say <text>` enqueues into Lane L1 (fast lane).
Worker outputs are emitted as Event kind `ANSWER` and visible via `events`.

Commands:
- `say <text>` : queue
- `lane` : lane telemetry
- `node` : node telemetry
- `metrics` : core counters
- `events [n]` : last events
