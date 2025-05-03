#include <stdlib.h>
#include "implication.h"
#include "tty.h"

/*
 * Initialize the implication table.
 */
void
initimplic(const BOOL * bornrules, const BOOL * liverules, FLAGS * implic)
{
    int nunk, non, noff, cunk, con, coff, funk, fon, foff, naon, caon, faon, desc;
    BOOL valid, cison, cisoff, fison, fisoff, nison, nisoff;

    for (desc=0; desc<IMPLICSIZE; desc++) {
        implic[desc] = IMPVOID;
    }

    for (nunk=0; nunk<=8; nunk++) { // unknown neighbors
        for (non=0; non+nunk<=8; non++) { // on neighbors from the known ones
            noff=8-(non+nunk); // off known neighbors
            for (cunk=0; cunk<=1; cunk++) { // unknown cell
                for (con=0; con+cunk<=1; con++) { // on cell
                    coff=1-(con+cunk); // off cell
                    for (funk=0; funk<=1; funk++) { // unknown future cell
                        for (fon=0; fon+funk<=1; fon++) { // on future cell
                            foff=1-(fon+funk); // off future cell
                            desc = SUMTODESC((STATE)(funk*UNK+fon*ON+foff*OFF), (STATE)(cunk*UNK+con*ON+coff*OFF), nunk*UNK+non*ON+noff*OFF);
                            if (implic[desc] != IMPVOID) {
                                ttystatus("Duplicate descriptor!!!");
                                exit(1);
                            }
                            // here we get all possible descriptors
                            // now let's try all possible states for that descriptor
                            valid = FALSE; // will change to TRUE if we get at least one valid state
                            cison = TRUE; // will change to FALSE if we get a valid state with c cell OFF
                            cisoff = TRUE; // will change to FALSE if we get a valid state with c cell ON
                            fison = TRUE; // will change to FALSE if we get a valid state with f cell OFF
                            fisoff = TRUE; // will change to FALSE if we get a valid state with f cell ON
                            nison = TRUE; // will change to FALSE if we get a valid state with one unknown neighbor OFF
                            nisoff = TRUE; // will change to FALSE if we get a valid state with one unknown neighbor ON
                            for (naon = non; naon <= 8-noff; naon++) { // neighbors
                                for (caon = con; caon <= 1-coff; caon++) { // try center
                                    for (faon = fon; faon <= 1-foff; faon++) { // try future
                                        // here we have all possible states for the descriptor
                                        // now for the rules
                                        if (((caon == 0) && (faon == 0) && !bornrules[naon]) // both dead
                                            || ((caon != 0) && (faon == 0) && !liverules[naon]) // dying
                                            || ((caon == 0) && (faon != 0) && bornrules[naon]) // birth
                                            || ((caon != 0) && (faon != 0) && liverules[naon])) { // survival
                                            // woohoo! we got a valid state
                                            valid = TRUE;
                                            if (caon == 0) {
                                                cison = FALSE;
                                            } else {
                                                cisoff = FALSE;
                                            }
                                            if (faon == 0) {
                                                fison =  FALSE;
                                            } else {
                                                fisoff = FALSE;
                                            }
                                            if (naon>non) nisoff = FALSE;
                                            if (naon<non+nunk) nison = FALSE;
                                        }
                                    }
                                }
                            }
                            // descriptor examination has ended
                            // now for the results
                            if (!valid) {
                                implic[desc] = IMPBAD;
                            } else {
                                implic[desc] = IMPOK;
                                if (funk != 0) { // future cell is unknown
                                    if (fison || fisoff) { // and just one state is possible
                                        implic[desc] |= IMPN;
                                        if (fison) {
                                            implic[desc] |= IMPN1;
                                        }
                                    }
                                }
                                if (cunk != 0) {
                                    if (cison || cisoff) {
                                        implic[desc] |= IMPC;
                                        if (cison) {
                                            implic[desc] |= IMPC1;
                                        }
                                    }
                                }
                                if (nunk != 0) {
                                    if (nison || nisoff) {
                                        implic[desc] |= IMPUN;
                                        if (nison) {
                                            implic[desc] |= IMPUN1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    for (desc=0; desc<sizeof(implic)/sizeof(implic[0]); desc++) {
        if (implic[desc] == IMPVOID) {
            implic[desc] = IMPBAD;
        }
    }

}

