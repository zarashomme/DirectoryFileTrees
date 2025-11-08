/*--------------------------------------------------------------------*/
/* nodeFT.c                                                           */
/* Author: Zara Hommez                                                */
/*--------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "dynarray.h"
#include "nodeFT.h"
#include "checkerFT.h"

struct node {
    /* the object corresponding to the node's absolute path */
   Path_T oPPath;
   /* this node's parent */
   Node_T oNParent;
   /* the object containing links to this node's children */
   DynArray_T oDChildren;
   /* boolean to store if file (true) or directory (false)*/
   boolean bIsFile;
   /* pointer to the contents of file (null if directory)*/
   void *pvContents;
   /* leangth of the contents of file ( 0 if directory)*/
   size_t ulContentLength;
};

/* Links new child oNChild into oNParent's children array at index
   ulIndex. Returns SUCCESS if the new child was added successfully,
   or MEMORY_ERROR if allocation fails. */
static int Node_addChild(Node_T oNParent, Node_T oNChild, 
   size_t ulIndex) {
   assert(oNParent != NULL);
   assert(oNChild != NULL);
   /* only directories can have children*/
   assert(!Node_isFile(oNParent));

   if(DynArray_addAt(oNParent->oDChildren, ulIndex, oNChild))
      return SUCCESS;
   else
      return MEMORY_ERROR;
}


static int Node_compareString(const Node_T oNFirst,
   const char *pcSecond) {
   assert(oNFirst != NULL);
   assert(pcSecond != NULL);

   return Path_compareString(oNFirst->oPPath, pcSecond);
}


int Node_new(Path_T oPPath, Node_T oNParent, boolean bIsFile,
             void *pvContents, size_t ulLength, Node_T *poNResult) {
   struct node *psNew;
   Path_T oPParentPath = NULL;
   Path_T oPNewPath = NULL;
   size_t ulParentDepth;
   size_t ulIndex = 0;
   int iStatus;

   assert(oPPath != NULL);
   assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));

   psNew = malloc(sizeof(struct node));
   if(psNew == NULL) {
      *poNResult = NULL;
      return MEMORY_ERROR;
   }

   iStatus = Path_dup(oPPath, &oPNewPath);
   if(iStatus != SUCCESS) {
      free(psNew);
      *poNResult = NULL;
      return iStatus;
   }
   psNew->oPPath = oPNewPath;


   if(oNParent != NULL) {
      size_t ulSharedDepth;

      /* verifies that parent is directory*/
      if(Node_isFile(oNParent)) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NOT_A_DIRECTORY;
      }

      /* already asserted parent isn't null*/
      oPParentPath = oNParent->oPPath;
      ulParentDepth = Path_getDepth(oPParentPath);
      ulSharedDepth = Path_getSharedPrefixDepth(psNew->oPPath,
                                                oPParentPath);

      /* should have a common depth since parent is the 
      prefix of the node*/
      if(ulSharedDepth < ulParentDepth) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return CONFLICTING_PATH;
      }

      /* parent should be direct ancestor(one less) than node*/
      if(Path_getDepth(psNew->oPPath) != ulParentDepth + 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }

      /* check if this node already exists*/
      if(Node_hasChild(oNParent, oPPath, &ulIndex)) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return ALREADY_IN_TREE;
      }
      
      iStatus = Node_addChild(oNParent, psNew, ulIndex);
      if(iStatus != SUCCESS) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return iStatus;
      }
   }
   else {
      /* parent is null, meaning this node needs to be new root*/
      if(Path_getDepth(psNew->oPPath) != 1 || bIsFile) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         if (bIsFile) 
            return CONFLICTING_PATH;
         else 
            return NO_SUCH_PATH;
      }
   }
   psNew->oNParent = oNParent;
   if (bIsFile) { 
      psNew->oDChildren = NULL;
      psNew->pvContents = pvContents; 
      psNew->ulContentLength = ulLength;
   }
   else { 
      psNew->oDChildren = DynArray_new(0);
      if(psNew->oDChildren == NULL) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return MEMORY_ERROR;
      }
      /* technically this assignment should be the case in
      contents/length handling in FT, but this is more explicit */
      psNew->pvContents = NULL; 
      psNew->ulContentLength = 0; 
   }
   psNew->bIsFile = bIsFile;
   *poNResult = psNew;

   assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));
   assert(CheckerFT_Node_isValid(*poNResult));

   return SUCCESS;
}

size_t Node_free(Node_T oNNode) {
   size_t ulIndex;
   size_t ulCount = 0;

   assert(oNNode != NULL);
   assert(CheckerFT_Node_isValid(oNNode));

   /* remove oNNode from children list of parent*/
   if(oNNode->oNParent != NULL) {
      Node_T oNParent = oNNode->oNParent;
   
      if(!Node_isFile(oNParent)) {
         if(DynArray_bsearch(oNParent->oDChildren, oNNode, &ulIndex,
                             (int (*)(const void *, const void *))
                                Node_compare))
            (void) DynArray_removeAt(oNParent->oDChildren, ulIndex);
      }
   }
   /* recursively remove children only of directories*/
   if(!Node_isFile(oNNode)) {
      while(DynArray_getLength(oNNode->oDChildren) != 0) {
         ulCount += Node_free(DynArray_get(oNNode->oDChildren, 0));
      }
      DynArray_free(oNNode->oDChildren);
   }

   Path_free(oNNode->oPPath);
   free(oNNode);
   ulCount++;

   return ulCount;
}

Path_T Node_getPath(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oPPath;
}

boolean Node_isFile(Node_T oNNode) {
    assert(oNNode != NULL);

    return oNNode->bIsFile;
}

boolean Node_hasChild(Node_T oNParent, Path_T oPPath, size_t *pulChildID) {
   assert(oNParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);
   /* ask preceptor about how to best handle*/
   assert(!Node_isFile(oNParent));

   return DynArray_bsearch(oNParent->oDChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) Node_compareString);
}

size_t Node_getNumChildren(Node_T oNParent) {
   assert(oNParent != NULL);

   if(Node_isFile(oNParent))
      return 0;

   return DynArray_getLength(oNParent->oDChildren);
}

int Node_getChild(Node_T oNParent, size_t ulChildID, Node_T *poNResult) {
   assert(oNParent != NULL);
   assert(poNResult != NULL);
   assert(!Node_isFile(oNParent));

   if(ulChildID >= Node_getNumChildren(oNParent)) {
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }

   *poNResult = DynArray_get(oNParent->oDChildren, ulChildID);
   return SUCCESS;
}

Node_T Node_getParent(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oNParent;
}

int Node_compare(Node_T oNFirst, Node_T oNSecond) {
   assert(oNFirst != NULL);
   assert(oNSecond != NULL);

   return Path_comparePath(oNFirst->oPPath, oNSecond->oPPath);
}

char *Node_toString(Node_T oNNode) {
   char *pcCopy;

   assert(oNNode != NULL);
   /* copy so that the caller can own this string rep. */
   pcCopy = malloc(Path_getStrLength(Node_getPath(oNNode)) + 1);
   if(pcCopy == NULL)
      return NULL;

   return strcpy(pcCopy, Path_getPathname(Node_getPath(oNNode)));
}

void *Node_getContents(Node_T oNNode) {
   assert(oNNode != NULL);

   if(Node_isFile(oNNode))
      return oNNode->pvContents;

   return NULL;
}

size_t Node_getContentLength(Node_T oNNode) {
   assert(oNNode != NULL);
   assert(Node_isFile(oNNode));

   if(Node_isFile(oNNode))
      return oNNode->ulContentLength;

   return 0;
}

void *Node_replaceContents(Node_T oNNode, void *pvNewContents,
                           size_t ulNewLength) {
   void *pvOldContents;

   assert(oNNode != NULL);
   assert(Node_isFile(oNNode));

   pvOldContents = oNNode->pvContents;
   oNNode->pvContents = pvNewContents;
   oNNode->ulContentLength = ulNewLength;

   return pvOldContents;
}


