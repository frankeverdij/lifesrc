#include "printblk.h"

static const char *colorBlockGfx[256] =
    {" ", "\u2580", "\u2584", "\u2588", "\033[32m\u2580\033[0m", " ", "\033[32;47m\u2580\033[0m", " ",
     "\033[32m\u2584\033[0m", "\033[32;47m\u2584\033[0m", " ", " ", "\033[32m\u2588\033[0m", " ", " ", " ",
     "\033[31m\u2580\033[0m", " ", "\033[31;47m\u2580\033[0m", " ", " ", " ", " ", " ",
     "\033[31;42m\u2580\033[0m", " ", " ", " ", " ", " ", " ", " ",
     "\033[31m\u2584\033[0m", "\033[31;47m\u2584\033[0m", " ", " ", " ", " ", "\033[31;42m\u2584\033[0m", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\033[31m\u2588\033[0m", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\033[36m\u2580\033[0m", " ", "\033[36;47m\u2580\033[0m", " ", " ", " ", " ", " ",
     "\033[36;42m\u2580\033[0m", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\033[36;41m\u2580\033[0m", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",

     "\033[36m\u2584\033[0m", "\033[36;47m\u2584\033[0m", " ", " ", "\033[36;42m\u2584\033[0m ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\033[36;41m\u2584\033[0m", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\033[36m\u2588\033[0m", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " "};

static const char *blockGfx[128] =
    {" ", "\u2580", "\u2584", "\u2588", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\U0001fb8e", " ", "\U0001fb92", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\U0001fb8f", "\U0001fb91", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\U0001fb90", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " "};

#define COLORUNK 4
#define COLORUCHK 16
#define COLORFRZ 64

void printBlk(const int gen, const Bool color)
{
    int row;
    int col;
    int twoStates, second;
    int up, down;
    
    for (row = 0; row < rowMax; row += 2)
    {
        for (col = 1; col <= colMax; col++)
        {
            if (color)
            {
                up = findCell(row + 1, col, gen);
                twoStates = cellTable[up];
                if (twoStates == UNK)
                {
                    twoStates = COLORUNK;
                    if (!(cellTable[up + O_GENFLAGS] & CHOOSECELL))
                        twoStates = COLORUCHK;
                    if (cellTable[up + O_GENFLAGS] & FROZENCELL)
                        twoStates = COLORFRZ;
                }

                if (row + 1 < rowMax)
                {
                    down = findCell(row + 2, col, gen);
                    second = cellTable[down];
                    if (second == UNK)
                    {
                        second = COLORUNK;
                        if (!(cellTable[down + O_GENFLAGS] & CHOOSECELL))
                            second = COLORUCHK;
                        if (cellTable[down + O_GENFLAGS] & FROZENCELL)
                            second = COLORFRZ;
                    }

                    twoStates += 2 * second;
                }

                ttyPrintf("%s", colorBlockGfx[twoStates]);

            } else {
                up = findCell(row + 1, col, gen);
                twoStates = cellTable[up];
                if (row + 1 < rowMax)
                {
                    down = findCell(row + 2, col, gen);
                    twoStates += 2 * cellTable[down];
                }
            
                ttyPrintf("%s", blockGfx[twoStates]);
            }
        }

        ttyWrite("\n", 1);
    }

    return;
}
