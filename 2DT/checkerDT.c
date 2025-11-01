/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author: Zara Hommez                                                */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"


/* Assume that the following simple getter functions work in all implementations,
 in order to make it less cumbersome to write your checks: 
 \Node_getPath, Node_getChild, Node_getParent, and Node_getNumChildren. 
 (You may also assume that the DynArray and Path modules are correct 
 in their entirety, though their design is fair game for part 2.5 below.)*/

/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
   /*parent node*/
   Node_T oNodeParent;
   /*oNNodes path*/
   Node_T oNodeChild;
   
   Path_T oNodePath;
   /*Parent of oNNodes path*/
   Path_T oParentPath;
   /*child of oNNodes path*/
   Path_T oChildPath;
   /*previous child's path for lexigraphic ordering check*/
   Path_T oPrevChildPath = NULL;
   /*current child index*/

   size_t ulChildIdx;
   /*ONNodes depth*/
   size_t ulNodeDepth;
   /*oNNodes parent depth*/
   size_t ulParentDepth;
   /*oNnodes child depth*/
   size_t ulChildDepth;

   /*index for looping through children*/
   Path_T oPrevChildPath;


   /
   /* Null Node Check: NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   /* path supposed to check details of path like is it starting with / or empty string etc. */

   /* for checks, it is assumed get parameters functionality 
   works to access internal fields of nodes*/
   oNodePath = Node_getPath(oNNode);
   ulNodeDepth = Path_getDepth(oNodePath);
   oNodeParent = Node_getParent(oNNode);
   oParentPath = Node_getPath(oNodeParent);
   
   if(oNodeParent && ulNodeDepth != 1) {
         fprintf(stderr, "A node has a NULL parent pointer that isn't the 
         root but has depth %lu\n", (unsigned long)ulNodeDepth);
         return FALSE; 
   }
   if(oNodeParent != NULL && ulNodeDepth == 1) {
      fprintf(stderr, "A non-root node has depth 1 but a non-NULL parent pointer\n");
      return FALSE; 
   }

   /* Null Path Check: Node must have a valid path, even root ex: "a/" */
   if(oNodePath == NULL) {
      fprintf(stderr, "A node has a NULL path\n");
      return FALSE; 
   }
   
   /* Invariant Empty Path Check: path must not be empty string */
   if(strcmp(Path_getPathname(oNodePath), "") == 0) {
      fprintf(stderr, "A node has an empty string as its path\n");
      return FALSE;
   }
   
   /* Invariant Depth Check: child's depth must be exactly one more than parent's depth */
      ulParentDepth = Path_getDepth(oParentPath);
      if(ulNodeDepth != ulParentDepth + 1) {
         fprintf(stderr, "Child depth is not parent depth + 1:\n
            Child: (%s) depth %lu , Parent: (%s) depth %lu\n",
                  Path_getPathname(oNodePath), (unsigned long)ulNodeDepth,
                  Path_getPathname(oParentPath), (unsigned long)ulParentDepth,
                );
         return FALSE;
      }

   /* Invariant Path Check: Parent's path must be the same path 
   without the current node included (i.e. depth node - 1 prefix)*/
   /* only need check if parent isn't root(NULL) */
   if(oNodeParent != NULL) {
      if(Path_getSharedPrefixDepth(oParentPath, oNodePath) !=
         ulNodeDepth - 1) {
         fprintf(stderr, "Incorrect parent-child path linkage:\n
            Parent node (%s) is not the immediate prefix path of child path (%s)\n",
            Path_getPathname(oParentPath), Path_getPathname(oNodePath));
         return FALSE;
      }
`  }

   /* Invariant Child Linkage Check: for each child of oNNode, 
   the child must link back to oNNode as its parent */
   oPrevChildPath = NULL;
   for(ulChildIdx = 0; ulChildIdx < Node_getNumChildren(oNNode); ulChildIdx++) {
      if(Node_getChild(oNNode, ulChildIdx, &oNodeChild) != SUCCESS) {
         fprintf(stderr, "Child at index %lu\n is not retrievable"
            , (unsigned long)ulChildIdx);
         return FALSE;
      }

      if(Node_getParent(oNodeChild) != oNNode) {
         fprintf(stderr, "Child's parent pointer does not point back to node 
            that has this node in its child array: "
            "Child path (%s) parent path (%s) vs this node path (%s)\n",
            Path_getPathname(Node_getPath(oNodeChild)),
            Path_getPathname(Node_getPath(Node_getParent(oNodeChild))),
            Path_getPathname(oNodePath));
         return FALSE;
      }

      oChildPath = Node_getPath(oNodeChild);
      ulChildDepth = Path_getDepth(oChildPath);
      /* Invariant Depth Check: 
      child's depth must be exactly one more than this node's depth */
      if(ulNodeDepth != ulChildDepth - 1) {
         fprintf(stderr, "Child depth is not parent depth + 1:\n
            Child: (%s) depth %lu , Parent: (%s) depth %lu\n",
                  Path_getPathname(oChildPath), (unsigned long)ulChildDepth,
                  Path_getPathname(oNodePath), (unsigned long)ulNodeDepth,
                );
         return FALSE;
      }  

      /* Invariant Path Check: 
      hild's path must have this node's path as a prefix with depth matching */
      if(Path_getSharedPrefixDepth(oNodePath, oChildPath) != ulNodeDepth) {
         fprintf(stderr, "Incorrect parent-child path linkage:\n
            Parent node (%s) is not the immediate prefix path of child path (%s)\n",
                 Path_getPathname(oNodePath), Path_getPathname(oChildPath));
         return FALSE;
      }

      

      /* Check: children must be in lexicographic order (sorted) and unique, 
      as prescribed by binary search insertion */
      if(oPrevChildPath != NULL) {
         int iCmp = Path_compareString(oPrevChildPath, Path_getPathname(oChildPath));
         if(iCmp > 0) {
            fprintf(stderr, "Children not in lexicographic order: 
               (%s) incorrectly precedes (%s)\n",
                    Path_getPathname(oPrevChildPath), Path_getPathname(oChildPath));
            return FALSE;
         }
         if(iCmp == 0) {
            fprintf(stderr, "Sibling nodes cannot have same name: (%s) appears twice\n",
                    Path_getPathname(oChildPath));
            return FALSE;
         }
      }
      oPrevChildPath = oChildPath;
   }

   return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.
   Counts the number of nodes found and stores it in *pulNodeCount.
*/
static boolean CheckerDT_treeCheck(Node_T oNNode, size_t *pulNodeCount) {
   size_t ulIndex;

   assert(pulNodeCount != NULL);

   if(oNNode!= NULL) {
      /* Count this node */
      (*pulNodeCount)++;

      /* Check on each node: node must be valid */
      if(!CheckerDT_Node_isValid(oNNode)){ 
         /* refer to CheckerDT_Node_isValid for printed error message*/
         return FALSE;
      }

      /* Traverse every child of current oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
      {
         Node_T oNodeChild = NULL;
         int iStatus = Node_getChild(oNNode, ulIndex, &oNodeChild);

         if(iStatus != SUCCESS) {
            fprintF(stderr, "Child at index %lu for node %s \n is not retrievable\n"
               , (unsigned long)ulIndex, Path_getPathname(Node_getPath(oNNode)));
            return FALSE;
         }

         /* if recurring down one subtree results in a failed check
            farther down, passes the failure back up immediately */
         /* this also recursively calls the next child*/
         if(!CheckerDT_treeCheck(oNodeChild, pulNodeCount))
            return FALSE;
      }
   }
   return TRUE;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {
   size_t ulCheckedCount = 0;

   /* DT Initialization Check: 
      if the DT is not initialized, its root(oNRoot) should be null and 
      node count(ulCount) should be 0 */
   if(!bIsInitialized) {
      if(oNRoot != NULL) {
         fprintf(stderr, "DT is not initialized, but root is not NULL\n");
         return FALSE;
      }
      if(ulCount != 0) {
         fprintf(stderr, "DT is not initialized, but node count is not 0\n");
         return FALSE;
      }
      /* all checks completed for uninitialized DT */ 
      return TRUE;
   }

   /* No root w non-zero count Invariant Check: 
   if initialized, root must not be NULL if count > 0 */
   if(ulCount > 0 && oNRoot == NULL) {
      fprintf(stderr, "Initialized with count %lu but root is NULL\n", 
         (unsigned long)ulCount);
      return FALSE;
   }

   /* Root w zero count Invariant Check: 
   root must be NULL if count is 0 */
   if(ulCount == 0 && oNRoot != NULL) {
      fprintf(stderr, "Count is 0 but root is not NULL\n");
      return FALSE;
   }

   /* Full Node Invariant Check: performs a preorder traversal to 
   recursively counts all nodes*/
   if(!CheckerDT_treeCheck(oNRoot, &ulCheckedCount)){ 
      /* refer to CheckerDT_treeCheck for printed error message*/
      return FALSE;
   }

   /* Check: the ulCount must equal 
   the actual number of nodes in the tree( ulCheckedCount)*/
   if(ulActualCount != ulCount) {
      fprintf(stderr, "Count inequality: tree has %lu nodes but ulCount is %lu \n",
              (unsigned long)ulCheckedCount, (unsigned long)ulCount);
      return FALSE;
   }

   return TRUE;
}
