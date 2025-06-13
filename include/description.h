#ifndef DESCRIPTION_H
#define DESCRIPTION_H

/*
 * Calculate descriptor for use with the implications table
 * 'a' is the state (0, 1 or 64, see 'state.h')
 * 'b' is the sum of the neighbors states
 */
#define SUMTODESC(a, b)  ((a) + 4 * (b))

/*
 * The maximum value for the above descriptor formula is 64 + 4 * 8 * 64
 * corresponding to a == 64 and b == 8 * 64, which is 33 * 64 == 2112.
 * Since we have an additional future state in the KS code and accounting
 * for the fact that this future state will be added to the descriptor as 2 * 8
 * the total is then 35 * 64 == 2240. This is the maximum descriptor index,
 * which means that the implication array should have at least 2241 entries.
 * Therefore i've chosen the array size to round up to 36 * 64, which is 2304.
 */
#define TRIMSIZE 2304

#endif /* DESCRIPTION_H */
