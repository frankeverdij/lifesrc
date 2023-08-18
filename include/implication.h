#ifndef IMPLICATION_H
#define IMPLICATION_H

void initImplic(const State * states, Flags * implic);
Flags implication(State state, int offCount, int onCount);

#endif /* IMPLICATION_H */

