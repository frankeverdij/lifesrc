#include "lifesrc.h"
#include "outputtimers.h"

void setSigaction(struct sigaction * act, const int sig, void * handler)
{
    sigemptyset(&(act->sa_mask));
    act->sa_flags = 0;
    act->sa_handler = handler;
    
    if (sigaction(sig, act, NULL) == -1)
    {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }

    return;
}

void createTimer(struct sigevent * sev, struct itimerspec * its, timer_t * tid, const int sig, const int sec)
{
    sev->sigev_notify = SIGEV_SIGNAL;
    sev->sigev_signo = sig;
    sev->sigev_value.sival_int = 0;
    sev->sigev_value.sival_ptr = NULL;
    
    if (timer_create(CLOCK_MONOTONIC, sev, tid) == -1)
    {
        perror("timer_create failed");
        exit(EXIT_FAILURE);
    }
    
    its->it_interval.tv_sec = sec; /* repeat timer events */
    its->it_interval.tv_nsec = 0;
    its->it_value.tv_sec = sec; /* first timer event */
    its->it_value.tv_nsec = 0;

    return;
}
