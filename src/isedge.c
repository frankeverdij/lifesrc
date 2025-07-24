#include "lifesrc.h"

Bool isEdge(const int row, const int col)
{
    Bool edge = 0;
        
    if (edgeDiagOffset > 0)
    {
        edge = ((row + col - 1 <= edgeDiagOffset) || ((rowMax + 1 - row) + (colMax + 1 - col) -1 <= edgeDiagOffset));
    }
    
    if (edgeDiagOffset < 0)
    {
        edge = ((row + (colMax + 1 - col) -1 <= -edgeDiagOffset) || ((rowMax + 1 - row) + col -1 <= -edgeDiagOffset));
    }

    return edge;
}
