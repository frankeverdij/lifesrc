#ifndef NEXTSTATE_H
#define NEXTSTATE_H

void initNextState(const State * bornRules, const State * liveRules);
State nextState(const State state, const int onCount);

#endif /* NEXTSTATE_H */
