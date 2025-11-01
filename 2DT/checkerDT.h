/*--------------------------------------------------------------------*/
/* checkerDT.h                                                        */
/* Author: Christopher Moretti                                        */
/*--------------------------------------------------------------------*/

#ifndef CHECKER_INCLUDED
#define CHECKER_INCLUDED

#include "nodeDT.h"

/*If bIsInitialized == FALSE, oNRoot must be NULL and ulCount == 0. 
If ulCount == 0, then oNRoot == NULL.
  If bIsInitialized == TRUE, ulCount must be the number of nodes in
  the hierarchy rooted at oNRoot (which may be NULL if ulCount == 0). 
  Returns TRUE if these conditions are met, FALSE otherwise, printing
  an explanation to stderr in that case.
*/


/*
   Returns TRUE if oNNode represents a directory entry
   in a valid state, or FALSE otherwise. Prints explanation
   to stderr in the latter case.
*/
boolean CheckerDT_Node_isValid(Node_T oNNode);

/*
   Returns TRUE if the hierarchy is in a valid state or FALSE
   otherwise.  Prints explanation to stderr in the latter case.
   The data structure's validity is based on a boolean
   bIsInitialized indicating whether the DT is in an initialized
   state, a Node_T oNRoot representing the root of the hierarchy, and
   a size_t ulCount representing the total number of directories in
   the hierarchy.
*/
boolean CheckerDT_isValid(boolean bIsInitialized,
                          Node_T oNRoot,
                          size_t ulCount);

#endif
