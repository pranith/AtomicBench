/*
 * barriers for x86 processors
 */

#ifndef _BARRIER_H_
#define _BARRIER_H_

void flush_cl(void *addr)
{
#if defined(__i386__) || defined(__x86_64__)
	asm volatile("clflush (%0)" :: "r"(addr));
#endif
}

#ifdef USE_BARRIER

#if defined(__i386__) || defined(__x86_64__)

#define read_barrier() \
	asm volatile("lfence":::"memory")

#define write_barrier() \
	asm volatile("sfence":::"memory")

#define barrier() \
	asm volatile ("mfence":::"memory")

static inline void flush(void *addr)
{
  asm volatile("clflush  (%0)" :: "r"(addr));
}

static inline void xchg(long *ptr, long dst)
{
  asm volatile("xchg %0, %1" : "+r"(dst), "+m"(*ptr):: "memory", "cc");
}

#elif defined(__arm__) || defined(__aarch64__)

static inline void xchg(long *ptr, long dst)
{
  __atomic_exchange_n(ptr, &dst, __ATOMIC_SEQ_CST);
}

static inline void flush(void *addr)
{
  asm volatile("dc cvac, %0" :: "r"(addr));
}

#define read_barrier() \
	asm volatile("dmb":::"memory")

#define write_barrier() \
	asm volatile("dmb ishst" :::"memory")

#define barrier() \
	asm volatile ("dmb ish" :::"memory")

#endif /* ARCH */

#else /* USE_BARRIER */

#define read_barrier() \
	asm volatile("mfence":::"memory")
#define write_barrier() do {} while(0)
#define barrier() \
	asm volatile("sfence":::"memory")

#endif /* USE_BARRIER */

#endif /* _BARRIER_H_ */
