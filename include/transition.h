#ifndef TRANSITION_H
#define TRANSITION_H

void initTransit(const State * states, State * transit);
State transition(State state, int offCount, int onCount);

#endif /* TRANSITION_H */
