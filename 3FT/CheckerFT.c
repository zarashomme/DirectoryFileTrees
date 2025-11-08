/*--------------------------------------------------------------------*/
/* checkerFT.c                                                        */
/* Author: Zara Hommez                                                */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "checkerFT.h"
#include "dynarray.h"
#include "path.h"


/* preceptor ask: decide if including arguements in error messages is worth it*/

/* see checkerFT.h for specification */
boolean CheckerFT_Node_isValid(Node_T oNNode) {
   Node_T oNodeParent = NULL;
   Node_T oNodeChild = NULL;
   Node_T oChildParent = NULL;

   Path_T oNodePath = NULL; 
   Path_T oParentPath = NULL;
   Path_T oChildPath = NULL;
   Path_T oPrevChildPath = NULL;
   
   size_t ulChildIdx = 0;
   size_t ulNodeDepth = 0;
   size_t ulParentDepth = 0;

   /* Null Node Check: NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   /* Node must have a valid path */
   oNodePath = Node_getPath(oNNode);
   if(oNodePath == NULL) {
      fprintf(stderr, "A node has a NULL path\n");
      return FALSE;
   }

   /* Empty path is invalid */
   if (strcmp(Path_getPathname(oNodePath), "") == 0) {
      fprintf(stderr, "A node has an empty string as its path\n");
      return FALSE;
   }

   ulNodeDepth = Path_getDepth(oNodePath);
   oNodeParent = Node_getParent(oNNode);
   
   if(oNodeParent != NULL) {
      /* Invariant: Files cannot have children*/
      if(Node_isFile(oNodeParent)) {
         fprintf(stderr, "File node incorrectly has child (%s)\n",
                 Path_getPathname(oNodePath));
         return FALSE;
      }

      if (ulNodeDepth == 1) {
         fprintf(stderr, "A node has a non-NULL parent but depth is 1\n");
         return FALSE;
      }
      oParentPath = Node_getPath(oNodeParent);
      ulParentDepth = Path_getDepth(oParentPath);

      if (Path_getSharedPrefixDepth(oParentPath, oNodePath) != ulNodeDepth - 1) {
         fprintf(stderr,
                 "Incorrect parent-child path linkage: Parent node (%s)" 
                 "is not the immediate prefix path of child path (%s)\n",
                 Path_getPathname(oParentPath), Path_getPathname(oNodePath));
         return FALSE;
      }
      
      if (ulNodeDepth != ulParentDepth + 1) {
         fprintf(stderr,
                 "Child depth is not parent depth + 1: Child:"
                 "(%s) depth %lu, Parent: (%s) depth %lu\n",
                 Path_getPathname(oNodePath), (unsigned long)ulNodeDepth,
                 Path_getPathname(oParentPath), (unsigned long)ulParentDepth);
         return FALSE;
      }
   }
   else {
      /* ask preceptor if this is a valid invariant or whether it can be*/
      if(Node_isFile(oNNode)) {
         fprintf(stderr, "Root cannot be a file: (%s)\n",
                 Path_getPathname(oNodePath));
         return FALSE;
      }
      if (ulNodeDepth != 1) {
         fprintf(stderr, "A non-root node has NULL parent but depth %lu\n",
                 (unsigned long)ulNodeDepth);
         return FALSE;
      }
   }

   if(Node_isFile(oNNode)) {
      if(Node_getNumChildren(oNNode) != 0) {
         fprintf(stderr, "File node incorrectly has children: (%s)\n",
                 Path_getPathname(oNodePath));
         return FALSE;
      }
      /* no further checks needed for file nodes since rest check children*/
      return TRUE;
   }

   /* oprevChild path? */
   for(ulChildIdx = 0; ulChildIdx < Node_getNumChildren(oNNode); ulChildIdx++) {
      int iStatus = Node_getChild(oNNode, ulChildIdx, &oNodeChild);
      if(iStatus != SUCCESS) {
         fprintf(stderr, "Child at index %lu is not retrievable\n", 
            (unsigned long)ulChildIdx);
         return FALSE;
      }

      if(Node_getParent(oNodeChild) != oNNode) {
         fprintf(stderr, "Child's parent mismatch under (%s)\n",
                 Path_getPathname(oNodePath));
         return FALSE;
      }

      oChildPath = Node_getPath(oNodeChild);
      if(Path_getSharedPrefixDepth(oNodePath, oChildPath) != ulNodeDepth) {
         fprintf(stderr, "Parent node (%s) returned a NULL child at index %lu\n", 
            Path_getPathname(oNodePath),(unsigned long)ulChildIdx);
         return FALSE;
      }

      oChildParent = Node_getParent(oNodeChild);
      if (oChildParent != oNNode) {
         const char *childPathStr = "(null)";
         const char *childParentPathStr = "(null)";
         const char *thisPathStr = "(null)";
         if (Node_getPath(oNodeChild) != NULL)
            childPathStr = Path_getPathname(Node_getPath(oNodeChild));
         if (oChildParent != NULL && Node_getPath(oChildParent) != NULL)
            childParentPathStr = Path_getPathname(Node_getPath(oChildParent));
         if (oNodePath != NULL)
            thisPathStr = Path_getPathname(oNodePath);
         fprintf(stderr,
                  "Child's parent pointer does not point back to node"
                 "that has this child: Child path (%s) parent path"
                 "(%s) vs this node path (%s)\n",
                 childPathStr, childParentPathStr, thisPathStr);
         return FALSE;
      }
      
      oChildPath = Node_getPath(oNodeChild);
      if (oChildPath == NULL) {
         fprintf(stderr, "A child has a NULL path\n");
         return FALSE;
      }

      ulChildDepth = Path_getDepth(oChildPath);

      if (ulNodeDepth != ulChildDepth - 1) {
         fprintf(stderr,
                 "Child depth is not parent depth + 1:"
                 "Child: (%s) depth %lu, Parent: (%s) depth %lu\n",
                 Path_getPathname(oChildPath), (unsigned long)ulChildDepth,
                 Path_getPathname(oNodePath), (unsigned long)ulNodeDepth);
         return FALSE;
      }

      /* since traversal handles the file-directory reordering, 
      this checker only needs to verify immediate lexigraphic consistent*/
      if(oPrevChildPath != NULL) {
         int iCmp = Path_comparePath(oPrevChildPath, oChildPath);
         if(iCmp == 0) {
               fprintf(stderr, "Sibling nodes cannot have same name: (%s) appears twice\n",
               Path_getPathname(oChildPath));
               return FALSE;
            }
         if (iCmp > 0) { 
            fprintf(stderr, "Children not in lexicographic order: (%s)"
            "incorrectly precedes (%s)\n",
            Path_getPathname(oPrevChildPath), Path_getPathname(oChildPath));
            return FALSE;
         }
      }

      oPrevChildPath = oChildPath;
   }

   return TRUE;
}

static boolean CheckerFT_treeCheck(Node_T oNNode, size_t *pulCount) {
   size_t ulIndex;

   assert(pulCount != NULL);
   
   if (oNNode == NULL)
      return TRUE;

   /* increment total amount of nodes */
   (*pulCount)++;


   /* verifies individual node is valid*/
   if(!CheckerFT_Node_isValid(oNNode))
      return FALSE;

   /* only directories should be checked for children traversal*/
   if(!Node_isFile(oNNode)) {
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++) {
         Node_T oNodeChild = NULL;

         int iStatus = Node_getChild(oNNode, ulIndex, &oNodeChild);
         if(iStatus != SUCCESS) {
            const char *thisPath = "(Null)";
            if (oNNode != NULL && Node_getPath(oNNode) != NULL) 
               thisPath = Path_getPathname(Node_getPath(oNNode));
            fprintf(stderr, "Child at index %lu for node %s is not retrievable\n",
               (unsigned long)ulIndex, thisPath);
            return FALSE;
          }

         if(!CheckerFT_treeCheck(oNodeChild, pulCount))
               return FALSE;
         }
      }
   return TRUE;
}


boolean CheckerFT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {
   size_t ulActualCount = 0;

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

   /* Root w zero count Invariant Check: 
   root must be NULL if count is 0 */
   if(ulCount == 0 && oNRoot != NULL) {
      fprintf(stderr, "Count is 0 but root is not NULL\n");
      return FALSE;
   }
   
   /* No root w non-zero count Invariant Check: 
   if initialized, root must not be NULL if count > 0 */
   if(ulCount > 0 && oNRoot == NULL) {
      fprintf(stderr, "Initialized with count %lu but root is NULL\n", 
         (unsigned long)ulCount);
      return FALSE;
   }

   /* preceptor: check if this is an actual invariant or allowed
   it would just be a 1 value tree*/
   if(oNRoot != NUll && Node_isFile(oNRoot)) {
      fprintf(stderr, "Root node is a file, which is invalid \n");
      return FALSE;
   }

   /* Full Node Invariant Check: performs a preorder traversal to 
   recursively counts all nodes*/
   if(!CheckerDT_treeCheck(oNRoot, &ulActualCount)){ 
      /* refer to CheckerDT_treeCheck for printed error message*/
      return FALSE;
   }

   
   /* Check: the ulCount must equal 
   the actual number of nodes in the tree( ulActualCount)*/
   if(ulActualCount != ulCount) {
      fprintf(stderr, "Count inequality: tree has %lu nodes but" 
         "ulCount is %lu \n", 
         (unsigned long)ulActualCount, (unsigned long)ulCount);
      return FALSE;
   }

   return TRUE;
}


