Atomic Bench
---

Simple benchmarks to measure the overhead of atomics.

Usage
---
	./bench <mem_size> <state1> <state2>

Here mem size is the size of the array in either bytes, kilo-bytes(K) or
mega-bytes(M). For example, 1024, 1K, 10K, 1M, 10M.

State is the state of the cache line. *state1* is the state the cache line
will be brought to in one core and *state2* is the state of the cache line
will be brought to subsequently. The supported states are

1. Modified state (STATE_M, 0)
2. Shared state (STATE_S, 1)

An example command line would be:
	./bench 1M 0 1
	
This would create two threads and the first thread will modify the memory
allocated bring them to modified state in its cache(STATE_M). After the first
thread is done, the second thread will access the same memory location with
reads to bring the cache line to shared state(STATE_S).
