// This file implements interfaces related to Pool Handling and Access Page
//  defined in buffer_mgr.h header.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer_mgr.h"
#include "storage_mgr.h"

// global variables
BufferPoolMgm_Info *bmi;

// initBufferPool creats a new buffer pool with numPages page frames using the page replacement strategy.
// The pool is used to cache pages from the page file with name pageFileName.
// -- Initially, all page frames should be empty.
// -- The page file should already exist.
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
                    const int numPages, ReplacementStrategy strategy,
		            void *stratData) 
{
    // check the validation of input parameters
    if(bm == NULL || pageFileName == NULL || numPages <= 0) {
        return RC_PARAMS_ERROR;
    }
    
    // check if the file specified by the filename exisits
    FILE *fp = fopen(pageFileName, "r+");
    if(fp == NULL) {
        return RC_FILE_NOT_FOUND;
    }
    fclose(fp);

    // create all frames and initialize them
    Frame**frames = (Frame**) calloc(numPages, sizeof(Frame*));
    for(int i = 0; i < numPages; i++) {
        // allocate memory for this frame
        Frame* frame = (Frame*)calloc(1, sizeof(Frame));
        frame->pageNum = NO_PAGE; 
        frame->pinCount = 0; 
        frame->dirtyBit = 0;
        frame->data = (char *)calloc(PAGE_SIZE, sizeof(char));
        frames[i] = frame;
    }

    // initialzie values of a new buffer pool 
    bm->pageFile = (char *) pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;
    // store frames info to the mgmtData
    bm->mgmtData = frames;    

    // initialize buffer pool management info
    bmi = (BufferPoolMgm_Info *)malloc(sizeof(BufferPoolMgm_Info));
    bmi->front = 0; // the index of the first frame
    bmi->rear = -1; // the index of the last frame
    bmi->frameCnt = 0;
    bmi->capacity = numPages;
    bmi->numRead = 0;
    bmi->numWrite = 0;
    if(strategy == RS_LRU) {
        int* arr =(int*)malloc(numPages * sizeof(int));
        for(int i = 0; i < numPages; i++) {
            arr[i] = -1;
        }
        bmi->arr = arr;
    }
    
    return RC_OK;

}

// shutdownBufferPool is to destory a buffer pool. 
// The method frees up all resources associated with buffer pool.
// -- Free the memory allocated for page frames.
// -- If the buffer pool contains any dirty pages, then these pages should be written back to disk before destroying.
// -- Raise an errot if a buffer pool has pinned pages.
RC shutdownBufferPool(BM_BufferPool *const bm)
{
    // check validation of input parameters
    if(bm == NULL) {
        return RC_PARAMS_ERROR;
    }
    // check whether the page file is null
    if(bm->pageFile == NULL)  
    {
        return RC_BUFFER_NOT_INIT;
    }
    // before release, forch flush pool
    forceFlushPool(bm);

    // release all frames data
    Frame** frames = (Frame**)bm->mgmtData;
    for(int i = 0; i < bm->numPages; i++) {
        if(frames[i]) {
            if(frames[i]->data) {
                free(frames[i]->data);
                frames[i]->data = NULL;
            }
            free(frames[i]);
            frames[i] = NULL;
        }
    }
    // free the arr of frames
    free(frames);
    frames = NULL;
    // release the bmi 
    if(bm->strategy == RS_LRU) {
        free(bmi->arr);
    }
    free(bmi);
    bmi = NULL;

    // shut down the buffer pool
    bm->pageFile = NULL;
    bm->numPages = NO_PAGE;
    // store frames info to the mgmtData
    bm->mgmtData = NULL;    
    return RC_OK;
}

// forceFlushPool is to cause all dirty pages from the buffer pool to be written to disk
// -- check whether there are dirty pages as well as the pin counts is equal to 0
RC forceFlushPool(BM_BufferPool *const bm)
{
    // check validation of input parameters
    if(bm == NULL) {
        return RC_PARAMS_ERROR;
    }
    
    if(bm->pageFile == NULL)  
    {
        return RC_BUFFER_NOT_INIT;
    }

    // open the file to be written
    SM_FileHandle fh;
	// open page file available on disk
	openPageFile(bm->pageFile, &fh);
    
    // get all frames in the buffer pool
    Frame** frames = (Frame**)bm->mgmtData;
    for(int i = 0; i < bm->numPages; i++) {
        if(frames[i]->pinCount == 0 && frames[i]->dirtyBit == 1) {
            // ensure the page num exists in the disk
            ensureCapacity(frames[i]->pageNum + 1, &fh);
			// write block of data to the page file on disk
			writeBlock(frames[i]->pageNum, &fh, frames[i]->data);
			// mark the page not dirty.
			frames[i]->dirtyBit = 0;
			// Increase the number of write times
			bmi->numWrite++;
        }
    }
    closePageFile(&fh);
    return RC_OK; 
}

// make a page as dirty
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // check validation of input parameters
    if(bm == NULL || page == NULL) {
        return RC_PARAMS_ERROR;
    }

    // make the page dirty and copy the page data to the frame 
    Frame** frames = (Frame**)bm->mgmtData;
    // search a frame from page cache
    Frame* frame = searchPageFromBuffer(frames, page->pageNum);

    // if this frame doesn't exist
    if(frame == NULL) {
        return RC_PAGE_NOT_EXIST;
    }
    frame->dirtyBit = 1;
    frame->data = page->data;
    return RC_OK;
}

// pinPage is to pin the page with page number pageNum. 
// pinning a page means that clients of the buffer mananger can request this page number.
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
		const PageNumber pageNum) 
{
    // check validation of input parameters
    if(bm == NULL || page == NULL || pageNum < 0) {
        return RC_PARAMS_ERROR;
    }

    // check whether the page file is null
    if(bm->pageFile == NULL)  
    {
        return RC_BUFFER_NOT_INIT;
    }

    // the requested page doesn't exist, execute different steps based on different strategies
    if(bm->strategy == RS_FIFO) {
        return pinPageWithFIFO(bm, page, pageNum);
    } else if(bm->strategy == RS_LRU) {
        return pinPageWithLRU(bm, page, pageNum);
    } else {
        return RC_NOT_SUPPORT_STRATEGY;
    }
    return RC_OK;
}

// unpins the page.
// The pageNum field of page is used to figure out which page to pin.
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // check validation of input parameters
    if(bm == NULL || page == NULL) {
        return RC_PARAMS_ERROR;
    }
    
    // search a frame from the buffer
    Frame** frames = (Frame**)bm->mgmtData;
    Frame* frame = searchPageFromBuffer(frames, page->pageNum);

     // if this frame doesn't exist
    if(frame == NULL) {
        return RC_PAGE_NOT_EXIST;;
    }
    // decrease the number of pin
    frame->pinCount--;
    if(bm->strategy == RS_LRU) {
        if(bmi->frameCnt == bmi->capacity) {
            updateLRUOrder(page->pageNum);
        }
        
    }
    if(frame->pinCount == 0 && frame->dirtyBit == 1) {
        forcePage(bm, page);
    }
    return RC_OK;
}

// forcePage is to write the current content of page back to the page file on disk.
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page) 
{
    // check validation of input parameters
    if(bm == NULL || page == NULL) {
        return RC_PARAMS_ERROR;
    }

    // force to flush the data to the disk
    Frame** frames = (Frame**)bm->mgmtData;

    // search a frame from page cache
    Frame* frame = searchPageFromBuffer(frames, page->pageNum);
    // if this frame doesn't exist
    if(frame == NULL) {
        return RC_PAGE_NOT_EXIST;
    }

    // open the page in the disk
    SM_FileHandle fh;

	openPageFile(bm->pageFile, &fh);

    ensureCapacity(page->pageNum + 1, &fh);

    // write this dirty page to the disk
    if(writeBlock(frame->pageNum, &fh, frame->data) != RC_OK) {
        return RC_WRITE_PAGE_FAILED;
    }
    closePageFile(&fh);

    return RC_OK;
    
}

RC pinPageWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page, 
		const PageNumber pageNum)
{
    // check validation of input parameters
    if(bm == NULL || page == NULL || pageNum < 0) {
        return RC_PARAMS_ERROR;
    }

    // get frames data
    Frame** frames = (Frame**)bm->mgmtData;

    // check whether the current page existed in the buffer pool
    Frame *frame = searchPageFromBuffer(frames, pageNum);
    if(frame != NULL) {
        page->pageNum = pageNum;
        page->data = frame->data;
        frame->pinCount++;
        return RC_OK;
    }

    // if the current page buffer is full and the requested page doesn't exist
    if (isFull()) {
        if(removePageWithFIFO(bm, page) != RC_OK) {
            return RC_NO_FREE_PIN_PAGE;
        }
    } 
    // get the frame to store this page content
    bmi->rear = (bmi->rear + 1) % bmi->capacity;
    frame = frames[bmi->rear];

    // copy the file content from disk to memory
    SM_FileHandle fh;
    openPageFile(bm->pageFile, &fh);
    if(ensureCapacity(pageNum + 1, &fh) != RC_OK) {
        return RC_READ_NON_EXISTING_PAGE;
    }
    if(readBlock(pageNum, &fh, frame->data) != RC_OK) {
        return RC_READ_PAGE_FALIED;
    }
    closePageFile(&fh);
    
    bmi->numRead++;
    
    // update this frame information page
    frame->pageNum = pageNum;
    frame->pinCount = 1;
    frame->dirtyBit = 0;

    // store page number info to page
    page->pageNum = pageNum;
    page->data = frame->data;
   
    // update the number of used frames
    bmi->frameCnt = bmi->frameCnt + 1;
    
    return RC_OK;
}

RC updateLRUOrder(int pageNum)
{
    // update hash
    int index = -1;
    int i;
    for(i = 0; i < bmi->capacity; i++) {
        if(bmi->arr[i] == pageNum) {
            index = i;
            break;
        }
    }

    if(index == -1) {
        return RC_PAGE_NOT_EXIST;
    }

    int updatePageNum = bmi->arr[index];
    for(i = index; i < bmi->capacity - 1; i++) {
        bmi->arr[i] = bmi->arr[i + 1];
    }
    bmi->arr[bmi->capacity - 1] = updatePageNum;
    return RC_OK;
}

Frame* removePageWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page, int leastUsedPage)
{
    // check whether this buffer pool is empty
    if (isEmpty())
        return NULL;

    // check whether there exisit frame with pinCount = 0
    int cnt = 0;
    Frame** frames = bm->mgmtData;
    for(int i = 0; i < bmi->capacity; i++) {
        if(frames[i]->pinCount == 0) {
            cnt++;
        }
    }
    if(cnt == 0) {
        return NULL;
    }
    
    // get the least page in the page cache
    Frame* frame = searchPageFromBuffer(frames, leastUsedPage);

    if(frame == NULL) {
        return NULL;
    }
 
    if(frame->pinCount == 0 && frame->dirtyBit == 1) {
        forcePage(bm, page);
        bmi->numWrite++;
    }

    // remove the least page
    frame->pageNum = NO_PAGE; 
    frame->pinCount = 0; 
    frame->dirtyBit = 0;

    bmi->frameCnt = bmi->frameCnt - 1;
    
    return frame;
}

RC pinPageWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page, 
		const PageNumber pageNum)
{
    // check validation of input parameters
    if(bm == NULL || page == NULL || pageNum < 0) {
        return RC_PARAMS_ERROR;
    }

    // get frames data
    Frame** frames = (Frame**)bm->mgmtData;

    // check whether the current page existed in the buffer pool
    Frame *frame = searchPageFromBuffer(frames, pageNum);
    if(frame != NULL) {
        page->pageNum = pageNum;
        page->data = frame->data;
        frame->pinCount++;
        updateLRUOrder(pageNum);
        return RC_OK;
    }

    int frameIndex = -1;
    // mark whether the current buffer pool is full
    int fullFlag = 0;
    if(isFull()) {
        int leastUsedPageNum = bmi->arr[0]; 
        frame = removePageWithLRU(bm, page, leastUsedPageNum);
        fullFlag = 1;
    } else {
        for(int i = 0; i < bm->numPages; i++) {
            if(bmi->arr[i] == -1) {
                frameIndex = i;
                break;
            }
        }
        if(frameIndex == -1) {
            return RC_PAGE_NOT_EXIST;
        } 
        frame = frames[frameIndex];
    }

    // copy the page content
    SM_FileHandle fh;

    openPageFile(bm->pageFile, &fh);
    
    // ensure the file page exists
    if(ensureCapacity(pageNum + 1, &fh) != RC_OK) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // copy the file content from disk to memory
    if(readBlock(pageNum, &fh, frame->data) != RC_OK) {
        return RC_READ_PAGE_FALIED;
    }

    bmi->numRead++;

    // update this frame information page
    frame->pageNum = pageNum;
    frame->pinCount = 1;
    frame->dirtyBit = 0;

    // store page number info to page
    page->pageNum = pageNum;
    page->data = frame->data;

    bmi->frameCnt = bmi->frameCnt + 1;

    // store this page in the cache
    if(fullFlag == 1) {
        for(int i = 0; i < bmi->capacity - 1; i++) {
            bmi->arr[i] = bmi->arr[i+1];
        }
        bmi->arr[bmi->capacity - 1] = pageNum;
    } else {
        bmi->arr[frameIndex] = pageNum;
    }
    closePageFile(&fh);
    return RC_OK;
}


// page cache is full when the frameCnt becomes equal to size
int isFull()
{
    return (bmi->frameCnt == bmi->capacity);
}

// page cache is empty when frameCnt is 0
int isEmpty()
{
    return (bmi->frameCnt == 0);
}


// Remove a frame from queue based on FIFO. It changes front and frameCnt
RC removePageWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // check whether this page cache is empty
    if (isEmpty()) {
        return RC_BUFFER_EMPTY;
    }
    // check whether there exist a frame with pinCount = 0
    Frame** frames = (Frame**)bm->mgmtData;
    int cnt = 0;
    for(int i = 0; i < bmi->capacity; i++) {
        if(frames[i]->pinCount == 0) {
            cnt++;
        }
    }
    if(cnt == 0) {
        return RC_NO_FREE_PIN_PAGE;
    }
    
    // get the first frame in the buffer pool
    Frame* frame = frames[bmi->front];
    // fix test case :201
    while(frame->pinCount > 0) {
        bmi->front = (bmi->front + 1) % bmi->capacity;
        frame = frames[bmi->front];
    } 
    if(bmi->front == 0) {
        bmi->rear = bmi->capacity - 1;
    } else {
        bmi->rear = bmi->front - 1;
    }
   
    if(frame->pinCount == 0 && frame->dirtyBit == 1) {
        forcePage(bm, page);
        bmi->numWrite++;
    }
    // remove the first frame
    bmi->front = (bmi->front + 1) % bmi->capacity;

    // update the number of used frame in page cache
    bmi->frameCnt = bmi->frameCnt - 1;

    // reset this frame node
    frame->pageNum = NO_PAGE; 
    frame->pinCount = 0; 
    frame->dirtyBit = 0;
    return RC_OK;
}

// get the frame from the page cache
Frame* searchPageFromBuffer(Frame **const frames, int pageNum) {
    // get a frame based on page number
    for(int i = 0; i < bmi->capacity; i++) {
        if(frames[i]->pageNum == pageNum) {
            return frames[i];
        }
    }
    return NULL;
}

// The getFrameContents function returns an array of PageNumbers (of size numPages) 
// where the ith element is the number of the page stored in the ith page frame. 
// An empty page frame is represented using the constant NO PAGE.
PageNumber *getFrameContents (BM_BufferPool *const bm) {
	if(bm == NULL) {
		return NULL;
	}
	int totalNumPages = bm->numPages;

	Frame** frames = (Frame**)bm->mgmtData;
	
	PageNumber *arr = (PageNumber*) malloc(bm->numPages * sizeof(PageNumber));
	int i;
	for(i = 0; i < bm->numPages;i++) {
		arr[i] = frames[i]->pageNum;
	}
	return arr;
}

// The getDirtyFlags function returns an array of bools (of size numPages) where the ith element
// is TRUE if the page stored in the ith page frame is dirty. Empty page frames are considered as clean.
int *getDirtyFlags (BM_BufferPool *const bm) {
	if(bm == NULL) {
		return NULL;
	}

	Frame** frames = (Frame**)bm->mgmtData;

	int numPages = bm->numPages;
	int *arr = (PageNumber*) malloc(numPages * sizeof(int));

	int i;
	for(int i = 0; i < numPages; i++) {
		arr[i] = frames[i]->dirtyBit;
        // printf("arr[%d] = %d\n", i, arr[i]);
	}
	return arr;
}

// The getFixCounts function returns an array of ints (of size numPages) where the ith element is
// the fix count of the page stored in the ith page frame. Return 0 for empty page frames.
int *getFixCounts (BM_BufferPool *const bm)
{
	if(bm == NULL) {
		return NULL;
	}

	Frame** frames = (Frame**)bm->mgmtData;
	int numPages = bm->numPages;
	int *arr = (PageNumber*) malloc(numPages * sizeof(int));

	int i;
	for(int i = 0; i < numPages; i++) {
		arr[i] = frames[i]->pinCount;
	}
	return arr;

}
int getNumReadIO (BM_BufferPool *const bm) {
	if(bm == NULL) {
		return -1;
	}
	return bmi->numRead;
}
int getNumWriteIO (BM_BufferPool *const bm) {
	if(bm == NULL) {
		return -1;
	}
	return bmi->numWrite;
}
