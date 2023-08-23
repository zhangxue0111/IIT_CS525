#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

// Include return codes and methods for logging errors
#include "dberror.h"

// Include bool DT
#include "dt.h"

// Replacement Strategies
typedef enum ReplacementStrategy {
  RS_FIFO = 0,
  RS_LRU = 1,
  RS_CLOCK = 2,
  RS_LFU = 3,
  RS_LRU_K = 4
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1

typedef struct BM_BufferPool {
  char *pageFile;
  int numPages;
  ReplacementStrategy strategy;
  void *mgmtData; // use this one to store the bookkeeping info your buffer
                  // manager needs for a buffer pool
} BM_BufferPool;

typedef struct BM_PageHandle {
  PageNumber pageNum;
  char *data;
} BM_PageHandle;

// Page Frame: each array entry in buffer pool
typedef struct Frame {
	PageNumber pageNum; // which page is currently stored in the frame
	int pinCount; // how many processes are using this page
	int dirtyBit; // whether the page has been modified
	char* data; // points to the area in memory storing the content of the page
} Frame;

// The used frame information
typedef struct BufferPoolMgm_Info {
	int front; // the first page in the buffer
	int rear; // the last page in the buffer
	int frameCnt; // the number of used frames in this buffer pool
	int capacity; // the total nuumber of frame the page cache can store
	int numRead; //stores number of pages that have been read
	int numWrite; //stores number of pages that been written
  int* arr;
}BufferPoolMgm_Info;


// convenience macros
#define MAKE_POOL()                 \
  ((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()              \
  ((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))

// Buffer Manager Interface Pool Handling
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
          const int numPages, ReplacementStrategy strategy,
          void *stratData);
extern RC shutdownBufferPool(BM_BufferPool *const bm);
extern RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
extern RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page);
extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page);
extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
        const PageNumber pageNum);

// Manage pages in buffer pool using FIFO
extern int isFull();
extern int isEmpty();
extern RC pinPageWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page, int pageNum); 
extern RC removePageWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page);

// Manage pages in buffer pool using LRU
extern RC pinPageWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum);
extern Frame* removePageWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page, int leastUsedPage);
extern RC updateLRUOrder(int pageNum);

extern Frame* searchPageFromBuffer(Frame** frames, int pageNum);   

#endif