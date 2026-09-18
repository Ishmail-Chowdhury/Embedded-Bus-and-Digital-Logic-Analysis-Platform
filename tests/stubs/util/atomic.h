#pragma once
// Native tests are single-threaded. Concurrent host writes are injected explicitly.
#define ATOMIC_RESTORESTATE 0
#define ATOMIC_BLOCK(x) for (bool atomicOnce = true; atomicOnce; atomicOnce = false)
