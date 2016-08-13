//#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <papi.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <atomic>

#include <string>

#define USE_BARRIER 1
#include "barrier.h"
#include "stat.h"

#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)

#define MEM_SIZE 16
#define CACHE_LINE_SIZE 64
#define REPEAT 1

volatile long *mem;

volatile int ready1, ready2, done;
long max_idx;
double ipc_result;

enum CacheLineState {
  STATE_M,
  STATE_S,
  STATE_I,
};

enum CacheLineState state1, state2;

void write_mem_atomic(void)
{
  volatile long dest = ~0;
  for (int i = 0; i < REPEAT; i++) {
    long index = 0;
    for (int j = 0; j < max_idx; j++) {
      //printf("%ld %ld\n", index, mem[index]);
      __atomic_exchange_n(mem+index, mem[index] & dest, __ATOMIC_SEQ_CST);
      index = mem[index];
    }
  }
  barrier();
}

void read_mem_atomic(void)
{
  volatile int index = 0;
  for (int j = 0; j < max_idx; j++) {
    index = __atomic_load_n(mem+index, __ATOMIC_SEQ_CST);
  }
}

void write_mem(void)
{
  volatile long dest = ~0;
  for (int i = 0; i < REPEAT; i++) {
    long index = 0;
    for (int j = 0; j < max_idx; j++) {
      //printf("%ld %ld\n", index, mem[index]);
      mem[index] = mem[index] & dest;
      index = mem[index];
    }
  }
  barrier();
}

void read_mem(void)
{
  volatile int index = 0;
  for (int j = 0; j < max_idx; j++) {
    index = mem[index];
  }
}

void flush_mem(void)
{
  for (int j = 0; j < max_idx; j++) {
    flush_cl((void *)(mem+j));
  }
}

void *current_thread(void *arg)
{
  enum CacheLineState state = *(enum CacheLineState *)arg; 
  long start, end;
  long long ins;
  float real_time, proc_time, ipc;

  while(!ready2);

  PAPI_ipc(&real_time, &proc_time, &ins, &ipc);
  //start = PAPI_get_real_cyc();
  switch (state) {
  case STATE_M:
    write_mem_atomic();
    break;
  case STATE_S:
    read_mem_atomic();
    break;
  case STATE_I:
    flush_mem();
    break;
  default:
    break;
  }
  //end = PAPI_get_real_cyc();
  PAPI_ipc(&real_time, &proc_time, &ins, &ipc);
  
  //printf("cyc: %ld, CPI: %ld\n", end-start, (end-start)/max_idx);
  //printf("Ins: %lld, IPC: %f\n", ins, ipc); 
  ipc_result = ipc;

  done = 1;

  return NULL;
}

void *other_thread(void *arg)
{
  enum CacheLineState state = *(enum CacheLineState *)arg; 

  while(!ready1);

  switch (state) {
  case STATE_M:
    write_mem_atomic();
    break;
  case STATE_S:
    read_mem_atomic();
    break;
  case STATE_I:
    flush_mem();
    break;
  default:
    break;
  }

  done = 1;

  return NULL;
}

void run_bench(void)
{
  pthread_t t1, t2;
  cpu_set_t cpuset1, cpuset2;
  struct timespec sleep_time, unslept_time;

  CPU_ZERO(&cpuset1);
  CPU_ZERO(&cpuset2);
  CPU_SET(1, &cpuset1);
  CPU_SET(1, &cpuset2);

  done = 0; ready1 = 0; ready2 = 0;

  pthread_create(&t1, NULL, &other_thread, &state1);
  pthread_setaffinity_np(t1, sizeof(cpu_set_t), &cpuset1);

  pthread_create(&t2, NULL, &current_thread, &state2);
  pthread_setaffinity_np(t2, sizeof(cpu_set_t), &cpuset2);

  // sleep for a while so that the frequencies are boosted
  sleep_time.tv_sec = 3; sleep_time.tv_nsec = 0;
  unslept_time.tv_sec = 0; unslept_time.tv_nsec = 0;
  do {
    int ret;
    if ((ret = clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID, 0, &sleep_time, &unslept_time)) != 0) {
      if (ret == EINVAL) {
	fprintf(stderr, "Wrong flags or arguments\n");
	exit(1);
      } else if (ret == EFAULT) {
	fprintf(stderr, "Invalid timerspec address\n");
	exit(1);
      } else if (ret == EINTR) {
	fprintf(stderr, "Interrupted sleep\n");
	sleep_time = unslept_time;
      }
    } else {
      unslept_time.tv_nsec = 0;
      unslept_time.tv_sec  = 0;
    }
  } while (unslept_time.tv_sec != 0 && unslept_time.tv_nsec != 0);

  ready1 = 1;
  
  while(!done);

  done = 0;
  ready2 = 1;

  while(!done);

  done = 0;
  
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
}

void generate_randindices()
{
  int i = 0;

  for (i = 0; i < max_idx; i++)
    mem[i] = -1;

  srandom(time(NULL));

  i = 0;
  while(i != max_idx) {
    int rand_index = random() % max_idx;
    if (mem[rand_index] == -1 && i != rand_index && mem[i] != rand_index) {
      mem[rand_index] = i;
      i++;
    }
  }

  return;
}

int calc_mem_size(std::string& mem_size_str)
{
  int mem_size;
  size_t foundK = mem_size_str.find("K");
  size_t foundM = mem_size_str.find("M");
  if (foundK != std::string::npos && foundM != std::string::npos) {
    fprintf(stderr, "Invalid mem size format\n");
    exit(1);
  }

  if (foundK != std::string::npos) {
    mem_size = KB(std::stoi(mem_size_str));
  } else if (foundM != std::string::npos) {
    mem_size = MB(std::stoi(mem_size_str));
  } else {
    mem_size = std::stoi(mem_size_str);
  }

  //fprintf(stderr, "Mem size: %d\n", mem_size);
  if (mem_size % 1024) {
    fprintf(stderr, "Mem size should be multiple of 1024\n");
    exit(1);
  }

  return mem_size;
}

int main(int argc, char **argv)
{
  int i = 0;
  int retval;
  double results[10];

  if (argc < 4) {
    fprintf(stderr, "Usage: %s <mem size> <state thread1> <state thread2>\n \
                     mem size in bytes/KB(K)/MB(M), state is STATE_M(0)/STATE_S(1)\n", argv[0]);
    exit(1);
  }

  std::string mem_size_str(argv[1]);

  int mem_size = calc_mem_size(mem_size_str);

  state1 = atoi(argv[2]) ? STATE_S : STATE_M;
  state2 = atoi(argv[3]) ? STATE_S : STATE_M;

  retval = PAPI_library_init(PAPI_VER_CURRENT);

  max_idx = mem_size/sizeof(long);
  mem = (long *)malloc(max_idx * sizeof(long));

  generate_randindices();

  for (i = 0; i < 10; i++) {
    run_bench();
    results[i] = ipc_result;
    flush_mem();
  }

  printf("%d, %f, %f\n", mem_size, get_mean(results, 10), get_stddev(results, 10));

  free((void *)mem);
  PAPI_shutdown();

  return 0;
}
