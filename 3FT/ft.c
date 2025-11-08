/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: Zara Hommez                                                */
/*--------------------------------------------------------------------*/

/*
  A File Tree is a representation of a hierarchy of directories and
  files: the File Tree is rooted at a directory, directories
  may be internal nodes or leaves, and files are always leaves.
*/

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "a4def.h"
#include "ft.h"
#include "path.h"
#include "dynarray.h"
#include "nodeFT.h"
#include "checkerFT.h"

/* 1. a flag for being in an initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. a pointer to the root node in the hierarchy */
static Node_T oNRoot;
/* 3. a counter of the number of nodes in the hierarchy */
static size_t ulCount;


/* --------------------------------------------------------------------

  The FT_traversePath and FT_findNode functions modularize the common
  functionality of going as far as possible down an FT towards a path
  and returning either the node of however far was reached or the  
  node if the full path was reached, respectively. 
*/

/*
  Traverses the FT starting at the root as far as possible towards
  absolute path oPPath. If able to traverse, returns an int SUCCESS
  status and sets *poNFurthest to the furthest node reached (which may
  be only a prefix of oPPath, or even NULL if the root is NULL).
  Furthermore, if a file is found( *pbFoundFile is TRUE), return an int 
SUCCESS status , with *poNFurthest set to the node of the file. 
Otherwise, sets *poNFurthest to NULL and returns with status:
  * CONFLICTING_PATH if the root's path is not a prefix of oPPath
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
/* short description for reference: 
traverse and return status , changing poNFurthest to node of 
farthest depth found of given path*/

static int FT_traversePath(Path_T oPPath, Node_T *poNFurthest,
                           boolean *pbFoundFile) {
   int iStatus;
   Path_T oPPrefix = NULL;
   Node_T oNCurr;
   Node_T oNChild = NULL
   size_t ulDepth;
   size_t i;
   size_t ulChildID;

   assert(oPPath != NULL);
   assert(poNFurthest != NULL);
   assert(pbFoundFile != NULL);
   *pbFoundFile = FALSE;

   /* root is NULL -> won't find anything */
   if(oNRoot == NULL) {
      *poNFurthest = NULL;
      return SUCCESS;
   }

   /* get first prefix of path (depth 1)*/
   iStatus = Path_prefix(oPPath, 1, &oPPrefix);
   if(iStatus != SUCCESS) {
      *poNFurthest = NULL;
      return iStatus;
   }

   /* if root path is not a prefix of path, return CONFLICTING_PATH */
   /* standardization with comparePath != 0 not specified in dtGood -> check*/
   if(Path_comparePath(Node_getPath(oNRoot), oPPrefix)) {
      Path_free(oPPrefix);
      *poNFurthest = NULL;
      return CONFLICTING_PATH;
   }
   Path_free(oPPrefix);
   oPPrefix = NULL;

   oNCurr = oNRoot;
   ulDepth = Path_getDepth(oPPath);

   for(i = 2; i <= ulDepth; i++) {
      if(Node_isFile(oNCurr)) {
         /* can't go further, found file */
         *poNFurthest = oNCurr;
         *pbFoundFile = TRUE;
         return SUCCESS;
      }

      iStatus = Path_prefix(oPPath, i, &oPPrefix);
      if(iStatus != SUCCESS) {
         *poNFurthest = NULL;
         return iStatus;
      }

      if(Node_hasChild(oNCurr, oPPrefix, &ulChildID)) {
         /* go to that child and continue with next prefix */
         Path_free(oPPrefix);
         oPPrefix = NULL;
         iStatus = Node_getChild(oNCurr, ulChildID, &oNChild);
         if(iStatus != SUCCESS) {
            *poNFurthest = NULL;
            return iStatus;
         }
         oNCurr = oNChild;
      }
      else {
         /* oNCurr doesn't have child with path oPPrefix:
            this is as far as we can go */
         break;
      }
   }

   Path_free(oPPrefix);
   oPPrefix = NULL;
   *poNFurthest = oNCurr;
   return SUCCESS;
}


/*
  Traverses the FT to find a node with absolute path pcPath. Returns a
  int SUCCESS status and sets *poNResult to be the node, if found.
  Otherwise, sets *poNResult to NULL and returns with status:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if no node with pcPath exists in the hierarchy
  * NOT_A_DIRECTORY if a proper prefix of pcPath exists as a file
  * MEMORY_ERROR if memory could not be allocated to complete request
 */
 /* short reference: returns node(of given path) if found and null if not w error status */

static int FT_findNode(const char *pcPath, Node_T *poNResult) {
   Path_T oPPath = NULL;
   Node_T oNFound = NULL;
   int iStatus;
   boolean bFoundFile = FALSE;

   assert(pcPath != NULL);
   assert(poNResult != NULL);
   /*don't have to assert bFoundFile this time since it isn't a pointer*/ 

   if(!bIsInitialized) {
      *poNResult = NULL;
      return INITIALIZATION_ERROR;
   }

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS) {
      *poNResult = NULL;
      return iStatus;
   }

   iStatus = FT_traversePath(oPPath, &oNFound, &bFoundFile);
   if(iStatus != SUCCESS) {
      Path_free(oPPath);
      *poNResult = NULL;
      return iStatus;
   }

   if(oNFound == NULL) {
      Path_free(oPPath);
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }

   /* check if paths not identical, meaning node wasn't found*/
   if(Path_comparePath(Node_getPath(oNFound), oPPath)) {
      Path_free(oPPath);
      *poNResult = NULL;
      /* file shouldn't have children to point to in path*/
      if(bFoundFile)
         return NOT_A_DIRECTORY;
      else
         return NO_SUCH_PATH;
   }

   Path_free(oPPath);
   *poNResult = oNFound;
   return SUCCESS;
}

/*--------------------------------------------------------------------*/

int FT_insertDir(const char *pcPath) {
   int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;
   boolean bFoundFile = FALSE;

   assert(pcPath != NULL);
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS)
      return iStatus;

   iStatus = FT_traversePath(oPPath, &oNCurr, &bFoundFile);
   if(iStatus != SUCCESS) {
      Path_free(oPPath);
      return iStatus;
   }
   /* check if prefix of path incorrectly is a file*/
   if(bFoundFile) {
      Path_free(oPPath);
      return NOT_A_DIRECTORY;
   }
   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
      /* if common ancestor isnt found that means they dont share root roo
      t( under assumption there is root now )
      if there is no root then its fine cause you are making one  */
   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }
   /* depth of provided path*/
   ulDepth = Path_getDepth(oPPath);
   /* new root */
   if(oNCurr == NULL)
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr)) + 1;
      /* exact path provided already found*/
      if(Path_comparePath(Node_getPath(oNCurr), oPPath) == 0) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   }

   /* from the already inserted prefix, builds and 
   connect node directories to form full path */
   while(ulIndex <= ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNNewNode = NULL;

      iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
         return iStatus;
      }

      iStatus = Node_new(oPPrefix, oNCurr, FALSE, NULL, 0, &oNNewNode);
      Path_free(oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
         return iStatus;
      }

      if(oNFirstNew == NULL)
         oNFirstNew = oNNewNode;
      oNCurr = oNNewNode;
      ulNewNodes++;
      ulIndex++;
   }

   Path_free(oPPath);

   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

boolean FT_containsDir(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);

   if(!bIsInitialized)
      return FALSE;

   iStatus = FT_findNode(pcPath, &oNFound);
   if(iStatus != SUCCESS)
      return FALSE;

   /* beyond node found, also have to verify if jts a directory,
    not a file */
   return (boolean) (!Node_isFile(oNFound));
}

int FT_rmDir(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;
   size_t ulRemoved;
   boolean bWasRoot;

   assert(pcPath != NULL);
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   /* critique of dtGood this wasnt there*/
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = FT_findNode(pcPath, &oNFound);
   if(iStatus != SUCCESS)
      return iStatus;

   if(Node_isFile(oNFound))
      return NOT_A_DIRECTORY;

   /* lowers the count of the # nodes removed in the subtree of oNfound*/
   ulCount -= Node_free(oNFound);
   /* if the count is zero, that means the node was the root and eliminated the tree*/
   if(ulCount == 0)
      oNRoot = NULL;
   
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

/* check oppath bs oppreix correct */
int FT_insertFile(const char *pcPath, void *pvContents,
                  size_t ulLength) {
   int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth;
   size_t ulIndex;
   size_t ulNewNodes = 0;
   boolean bFoundFile = FALSE;

   assert(pcPath != NULL);
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS)
      return iStatus;

   iStatus = FT_traversePath(oPPath, &oNCurr, &bFoundFile);
   if(iStatus != SUCCESS) {
      Path_free(oPPath);
      return iStatus;
   }

   if(bFoundFile) {
      Path_free(oPPath);
      return NOT_A_DIRECTORY;
   }

   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

   ulDepth = Path_getDepth(oPPath);
   /* new root */
   if(oNCurr == NULL)
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr)) + 1;

      /* path is already in the tree */
      if(ulIndex == ulDepth+1 && 
         Path_comparePath(Node_getPath(oNCurr), oPPath) == 0) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   }
   
   /* starting at oNCurr(current already found depth),
    build rest of the path one level at a time */
   while(ulIndex <= ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNNewNode = NULL;
      boolean bStopAtFile = (ulDepth == ulIndex);

      iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
         return iStatus;
      }

      // check to insert the final file or directories before 
      if (bStopAtFile) {
         iStatus = Node_new( oPPrefix, oNCurr, TRUE, pvContents, 
         ulLength, &oNNewNode);
      } 
      else {
         iStatus = Node_new(oPPrefix, oNCurr, FALSE, 
         NULL, 0, &oNNewNode);
      } 
     
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
         return iStatus;
      }
      Path_free(oPPrefix);
      ulNewNodes++;
      /* tracks first node for freeing if mem error*/
      if(oNFirstNew == NULL)
         oNFirstNew = oNNewNode;
      oNCurr = oNNewNode;
      ulIndex++;
   }

   Path_free(oPPath);

   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

boolean FT_containsFile(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);

   if(!bIsInitialized)
      return FALSE;

   iStatus = FT_findNode(pcPath, &oNFound);
   if(iStatus != SUCCESS)
      return FALSE;

   return (boolean) (Node_isFile(oNFound));
}

int FT_rmFile(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = FT_findNode(pcPath, &oNFound);
   if(iStatus != SUCCESS)
      return iStatus;

   /* verifies what you are removing is a file*/
   if(!Node_isFile(oNFound))
      return NOT_A_FILE;

   ulCount -= Node_free(oNFound);
   if(ulCount == 0) { 
      oNRoot = NULL;
   }

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

void *FT_getFileContents(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);

   /* return NULL for any reason can't obtain contents*/
   if(!bIsInitialized)
      return NULL;

   iStatus = FT_findNode(pcPath, &oNFound);
   if(iStatus != SUCCESS)
      return NULL;

   if(!Node_isFile(oNFound))
      return NULL;

   return Node_getContents(oNFound);
}

void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);

   if(!bIsInitialized)
      return NULL;

   iStatus = FT_findNode(pcPath, &oNFound);
   if(iStatus != SUCCESS)
      return NULL;

   if(!Node_isFile(oNFound))
      return NULL;
   
   return Node_replaceContents(oNFound, pvNewContents, ulNewLength);
}

int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   assert(pbIsFile != NULL);
   assert(pulSize != NULL);

   /* handles potential path problems*/
   iStatus = FT_findNode(pcPath, &oNFound);
   if(iStatus != SUCCESS)
      return iStatus;

   if(Node_isFile(oNFound)) {
      *pbIsFile = TRUE;
      *pulSize = Node_getContentLength(oNFound);
   }
   else {
      *pbIsFile = FALSE;
   }

   return SUCCESS;
}

int FT_init(void) {
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   /* can't initialize FT if already done*/
   if(bIsInitialized)
      return INITIALIZATION_ERROR;

   bIsInitialized = TRUE;
   oNRoot = NULL;
   ulCount = 0;

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

int FT_destroy(void) {
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   if(oNRoot != NULL) {
      ulCount -= Node_free(oNRoot);
      oNRoot = NULL;
   }

   bIsInitialized = FALSE;

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

/*--------------------------------------------------------------------*/

static size_t FT_preOrderTraversal(Node_T n, DynArray_T d, size_t i) {
   size_t nChildIdx;

   assert(d != NULL);

   if(n != NULL) {
      (void) DynArray_set(d, i, n);
      i++;

      if(!Node_isFile(n)) {
         
         size_t ulNumChildren = Node_getNumChildren(n);
         
         /* handles files and then directories at each 
         level through two seperate runs of the children 
         recursive calls */

         for(nChildIdx = 0; nChildIdx < ulNumChildren; nChildIdx++) {
            Node_T oNChild = NULL;
            int iStatus = Node_getChild(n, nChildIdx, &oNChild);
            assert(iStatus == SUCCESS);
            if(Node_isFile(oNChild))
               i = FT_preOrderTraversal(oNChild, d, i);
         }

         for(nChildIdx = 0; nChildIdx < ulNumChildren; nChildIdx++) {
            Node_T oNChild = NULL;
            int iStatus = Node_getChild(n, nChildIdx, &oNChild);
            assert(iStatus == SUCCESS);
            if(!Node_isFile(oNChild))
               i = FT_preOrderTraversal(oNChild, d, i);
         }
      }
   }

   return i;
}

static void FT_strlenAccumulate(Node_T oNNode, size_t *pulAcc) {
   assert(pulAcc != NULL);

   if(oNNode != NULL)
      *pulAcc += (Path_getStrLength(Node_getPath(oNNode)) + 1);
}

static void FT_strcatAccumulate(Node_T oNNode, char *pcAcc) {
   assert(pcAcc != NULL);

   if(oNNode != NULL) {
      strcat(pcAcc, Path_getPathname(Node_getPath(oNNode)));
      strcat(pcAcc, "\n");
   }
}

/*--------------------------------------------------------------------*/

char *FT_toString(void) {
   DynArray_T nodes;
   size_t totalStrlen = 1;
   char *result = NULL;

   if(!bIsInitialized)
      return NULL;

   nodes = DynArray_new(ulCount);
   (void) FT_preOrderTraversal(oNRoot, nodes, 0);

   DynArray_map(nodes, (void (*)(void *, void *)) FT_strlenAccumulate,
                (void *) &totalStrlen);

   result = malloc(totalStrlen);
   if(result == NULL) {
      DynArray_free(nodes);
      return NULL;
   }
   *result = '\0';

   DynArray_map(nodes, (void (*)(void *, void *)) FT_strcatAccumulate,
                (void *) result);

   DynArray_free(nodes);

   return result;
}

