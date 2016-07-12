#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#define DEBUG 0

unsigned long long get_timer_nsec(struct timespec *timer)
{
  return timer->tv_sec * 1E6 + timer->tv_nsec / 1000;
}

unsigned long long get_timer_usec(struct timeval *tv)
{
  return tv->tv_sec * 1E6 + tv->tv_usec;
}

long get_timer_diff(struct timespec *before, struct timespec *after)
{
  return get_timer_nsec(after) - get_timer_nsec(before);
}

unsigned long long get_timer_diff_usec(struct timeval *before, struct timeval* after) 
{
  unsigned long long before_usec = get_timer_usec(before);
  unsigned long long after_usec = get_timer_usec(after);
  assert(after_usec > before_usec);
  return get_timer_usec(after) - get_timer_usec(before);
}

void start_watch_usec(struct timeval *tv) {
  if (gettimeofday(tv, NULL)) {
    fprintf(stderr, "startWatch failed\n");
  }
}


void stop_watch_usec(struct timeval *tv) {
  start_watch_usec(tv);
}

void start_watch(struct timespec *timer)
{
  if (clock_gettime(CLOCK_MONOTONIC, timer))
    fprintf(stderr, "startWatch failed\n");

#if DEBUG
  printf("time val is %llu\n", get_timer_nsec(timer));
#endif
}

void stop_watch(struct timespec *timer)
{
  return start_watch(timer);
}

double get_mean(double *results, int num)
{
  double sum = 0;
  for (int i = 0; i < num; i++)
    sum += results[i];

  return sum / num;
}

double get_stddev(double *results, int num)
{
  double sum2 = 0;
  double m = get_mean(results, num);
  for (int i = 0; i < num; i++) {
    assert(results[i] > 0);
    double diff = results[i] - m;
    sum2 += diff * diff;
  }

  double variance = sum2 / (num - 1);

  return sqrt(variance);
}
