#ifndef OUTPUTTIMERS_H
#define OUTPUTTIMERS_H

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

void setSigaction(struct sigaction * act, const int sig, void * handler);
void createTimer(struct sigevent * sev, struct itimerspec * its, timer_t * tid, const int sig, const int sec);

#endif /* OUTPUTTIMERS_H */
