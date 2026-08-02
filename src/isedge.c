#include "lifesrc.h"

Bool isEdge(const int row, const int col, const int offset)
{
    Bool edge = 0;
        
    if (offset > 0)
    {
        edge = ((row + col - 1 <= offset) || ((rowMax + 1 - row) + (colMax + 1 - col) -1 <= offset));
    }
    
    if (offset < 0)
    {
        edge = ((row + (colMax + 1 - col) -1 <= -offset) || ((rowMax + 1 - row) + col -1 <= -offset));
    }

    return edge;
}
