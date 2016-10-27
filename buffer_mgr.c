#include "buffer_mgr.h"
#include "dberror.h"
#include "storage_mgr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


PIN_PAGE *firstPage=NULL;

RC updateBMPageHandle(PageNumber , char * ,BM_PageHandle *);

SM_FileHandle *fh;
BM_BufferPool bm_BufferPool;
BM_PageHandle bm_PageHandle;
SM_PageHandle ph;
int numReadIO;
int numWriteIO;
int buffFrameCount=0;
int bufferFrameSize;
int *buffPageLookUpTbl;
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy,
                  void *stratData){

    if(bm==NULL)
        return RC_BM_SPC_ALLOC_FAILED;

    fh = MAKE_SM_FILE_HANDLE();

    numReadIO = 0;
    numWriteIO =0;

    bm->pageFile = pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;
    bm->mgmtData = stratData;

    bufferFrameSize = numPages;
    int frameLookUpTbl[bufferFrameSize];
    for(int i=0; i<bufferFrameSize; i++)
        frameLookUpTbl[i]=-1;


    buffPageLookUpTbl=frameLookUpTbl;


    if(firstPage==NULL)
        firstPage = createBufferOfSize(numPages,firstPage);
    if(firstPage!=NULL)
        printf("\n Buffer created successfully ");


    int rtnCod=openPageFile(pageFileName,fh);
    printf(" \n returen code :: %d ",fh->totalNumPages);
    return RC_OK;
}
RC shutdownBufferPool(BM_BufferPool *const bm){
    PIN_PAGE *tempPage;
    ph = (SM_PageHandle)malloc(PAGE_SIZE);
    int pageLoc[bufferFrameSize];
    returnPagePosition(firstPage,&pageLoc);
    buffPageLookUpTbl = &pageLoc;
    printf("\n inside shutdown buffer");
    for(int i=0; i<bufferFrameSize; i++){
        if(pageLoc[i]!=-1){
            int indexFrame = isPagePresent(pageLoc[i]);
            if(indexFrame==-1)
                return RC_IVLD_PAGE_NUM;
            tempPage = getFrameFromLoc(firstPage,indexFrame);
            if(tempPage->fixCount ==0){
                if(tempPage->isDirty==TRUE){
                    ph = (SM_PageHandle)malloc(PAGE_SIZE);
                    ph =  tempPage->data;
                    if(RC_OK != writeBlock(tempPage->pageNum,fh,ph)){
                        return RC_WRITE_FAILED;
                    }
                    tempPage->isDirty=FALSE;
                    numWriteIO = numWriteIO + 1;
                }
            }else{
                return RC_BUFFER_IN_USE;
            }
        }
    }

    printf("shutdown success");
    return RC_OK;
}
RC forceFlushPool(BM_BufferPool *const bm){
    PIN_PAGE *tempPage;
    ph = (SM_PageHandle)malloc(PAGE_SIZE);
    int pageLoc[bufferFrameSize];
    returnPagePosition(firstPage,&pageLoc);
    buffPageLookUpTbl = &pageLoc;
    printf("\n inside shutdown buffer");
    for(int i=0; i<bufferFrameSize; i++){
        if(pageLoc[i]!=-1){
            int indexFrame = isPagePresent(pageLoc[i]);
            if(indexFrame==-1)
                return RC_IVLD_PAGE_NUM;
            tempPage = getFrameFromLoc(firstPage,indexFrame);
            if(tempPage->fixCount ==0){
                if(tempPage->isDirty==TRUE){
                    ph = (SM_PageHandle)malloc(PAGE_SIZE);
                    ph =  tempPage->data;
                    if(RC_OK != writeBlock(tempPage->pageNum,fh,ph)){
                        return RC_WRITE_FAILED;
                    }
                    tempPage->isDirty=FALSE;
                    numWriteIO = numWriteIO + 1;
                }

        }
    }
    return RC_OK;
}

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){
    PIN_PAGE *tempPage;
    ph = (SM_PageHandle)malloc(PAGE_SIZE);
    int pageLoc[bufferFrameSize];
    returnPagePosition(firstPage,&pageLoc);
    buffPageLookUpTbl = &pageLoc;
    int indexFrame = isPagePresent(page->pageNum);
    if(indexFrame==-1)
        return RC_IVLD_PAGE_NUM;

    tempPage = getFrameFromLoc(firstPage,indexFrame);
    tempPage->isDirty = TRUE;

    return RC_OK;
}
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){
    PIN_PAGE *tempPage;
    ph = (SM_PageHandle)malloc(PAGE_SIZE);
    int pageLoc[bufferFrameSize];
    returnPagePosition(firstPage,&pageLoc);
    buffPageLookUpTbl = &pageLoc;
    int indexFrame = isPagePresent(page->pageNum);
    if(indexFrame==-1)
        return RC_IVLD_PAGE_NUM;
    else
        printf("\n before unpined %d",tempPage->fixCount);
        tempPage = updateFixCountAtFrameLoc(firstPage,indexFrame,-1);
        page->data = tempPage->data;
        printf("\n unpined success %d",tempPage->fixCount);
    return RC_OK;
}
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){

    PIN_PAGE *tempPage;
    ph = (SM_PageHandle)malloc(PAGE_SIZE);
    int pageLoc[bufferFrameSize];
    returnPagePosition(firstPage,&pageLoc);
    buffPageLookUpTbl = &pageLoc;
    int indexFrame = isPagePresent(page->pageNum);
    if(indexFrame==-1)
        return RC_IVLD_PAGE_NUM;

    tempPage = getFrameFromLoc(firstPage,indexFrame);

    if(tempPage->fixCount ==0){
        if(tempPage->isDirty==TRUE){
            ph = (SM_PageHandle)malloc(PAGE_SIZE);
            ph =  tempPage->data;
            if(RC_OK != writeBlock(tempPage->pageNum,fh,ph)){
                return RC_WRITE_FAILED;
            }
            tempPage->isDirty=FALSE;
            numWriteIO = numWriteIO + 1;
        }
    }else{
        return RC_BUFFER_IN_USE;
    }
    printf("\n forcepage success");
    return RC_OK;
}
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
            const PageNumber pageNum){
    ensureCapacity(pageNum+1,fh);

    PIN_PAGE *tempPage;
    ph = (SM_PageHandle)malloc(PAGE_SIZE);
    int *newLookup;
    int pageLoc[bufferFrameSize];
    newLookup = returnPagePosition(firstPage,&pageLoc);
    buffPageLookUpTbl = &pageLoc;

    int indexFrame = isPagePresent(pageNum);
    if(indexFrame != -1){
        printf("Page Hit");
        tempPage = updateFixCountAtFrameLoc(firstPage,indexFrame,1);
        page->data = tempPage->data;
        page->pageNum = pageNum;
        return RC_OK;
    }
    indexFrame = getIndexPageByFIFO();
    if(indexFrame ==-1)
        return RC_UNKNOWN_ERROR;
    else{
        boolean isDirty = isPageDirty(firstPage,indexFrame);
        if(isDirty){
            tempPage = getFrameFromLoc(firstPage,indexFrame);
            ph = (SM_PageHandle)malloc(PAGE_SIZE);
            ph =  tempPage->data;
            if(RC_OK != writeBlock(tempPage->pageNum,fh,ph)){
                return RC_WRITE_FAILED;
            }
        }
        ph = (SM_PageHandle)malloc(PAGE_SIZE);
        if (RC_OK != readBlock(pageNum, fh, ph)){
            printf("\n page to be read %d ",pageNum);
            printf("\n total no of pages %d",fh->totalNumPages);
            return RC_READ_FAILED;
        }
        tempPage = getFrameFromLoc(firstPage,indexFrame);
        tempPage->fixCount =1;
        tempPage->isDirty = FALSE;
        sprintf(ph, "%s-%i", "Page", pageNum);
        page->data =ph;
        page->pageNum = pageNum;
        buffPageLookUpTbl[indexFrame]=pageNum;
        updatePinPageAtLoc(firstPage,indexFrame,pageNum,ph);

        firstPage = moveHeadToEnd(firstPage);
        shiftPageLookTbl();

        numReadIO = numWriteIO + 1;
    }
    return RC_OK;
}

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm){
    return RC_OK;
}
bool *getDirtyFlags (BM_BufferPool *const bm){
    return RC_OK;
}
int *getFixCounts (BM_BufferPool *const bm){
    return RC_OK;
}
int getNumReadIO (BM_BufferPool *const bm){
    return RC_OK;
}
int getNumWriteIO (BM_BufferPool *const bm){
    return RC_OK;
}


RC updateBMPageHandle(PageNumber pageNum, char *data ,BM_PageHandle *bmPageHandle){
    bmPageHandle->pageNum = pageNum;
    bmPageHandle->data = data;
    return RC_OK;
}

PIN_PAGE* createBufferOfSize(int numPages,PIN_PAGE *head){
    for(int i=0;i<numPages;i++){
        firstPage= createFrame(head);
        head=firstPage;
    }
    return head;
}

PIN_PAGE* createFrame(PIN_PAGE *head){
    PIN_PAGE *tempPage;

    if(head==NULL){
        head = malloc(sizeof(PIN_PAGE));
        head->nxt_pin_page=NULL;
        head->data = NULL;
        head->pageNum = -1;
        head->isDirty = FALSE;
        head->fixCount = 0;
        return head;
    }
    tempPage=head;
    while(tempPage->nxt_pin_page!=NULL)
        tempPage=tempPage->nxt_pin_page;
    tempPage->nxt_pin_page= malloc(sizeof(PIN_PAGE));
    tempPage->nxt_pin_page->pageNum = -1;
    tempPage->nxt_pin_page->data = NULL;
    tempPage->nxt_pin_page->isDirty=FALSE;
    tempPage->nxt_pin_page->fixCount=0;
    tempPage->nxt_pin_page->nxt_pin_page=NULL;
    return head;
}

PIN_PAGE* moveHeadToEnd(PIN_PAGE *head){
    PIN_PAGE *tempPage=head;
    PIN_PAGE *lastPage=head;

    while(lastPage->nxt_pin_page!=NULL)
        lastPage= lastPage->nxt_pin_page;

    head=head->nxt_pin_page;

    lastPage->nxt_pin_page = tempPage;
    lastPage->nxt_pin_page->nxt_pin_page = NULL;
    return head;
}

PIN_PAGE* moveFrameCurtLocToEnd(PIN_PAGE *head,int frameIndex){
    PIN_PAGE *tempPage=head;
    PIN_PAGE *lastPage=head;
    PIN_PAGE *prevPage;
    int counter=0;
    if(frameIndex==0)
        return moveHeadToEnd(head);
    if(frameIndex==(bufferFrameSize-1))
        return head;

    while(counter<frameIndex){
        prevPage=tempPage;
        tempPage=tempPage->nxt_pin_page;
        counter++;
    }

    while(lastPage->nxt_pin_page!=NULL)
        lastPage= lastPage->nxt_pin_page;

    prevPage->nxt_pin_page=tempPage->nxt_pin_page;

    lastPage->nxt_pin_page = tempPage;
    lastPage->nxt_pin_page->nxt_pin_page = NULL;
    return head;
}
PIN_PAGE* updatePageInfo(PIN_PAGE *page,char *pageData, boolean isDirtyFlag, int fixCount, int pageNum){
    page->data=pageData;
    page->isDirty=isDirtyFlag;
    page->fixCount=fixCount;
    page->pageNum=pageNum;

    return page;
}

PIN_PAGE* updatePinPageAtLoc(PIN_PAGE *head,int frameIndex,int pageNum, char *newData){
    PIN_PAGE *tempPage=head;
    int counter=0;
    while(counter<frameIndex){
        tempPage=tempPage->nxt_pin_page;
        counter++;
    }
    tempPage->data=newData;
    tempPage->pageNum= pageNum;

    return head;
}


void shiftPageLookTbl(){
    int firstFrame =buffPageLookUpTbl[0];
    for(int i=0; i<(bufferFrameSize-1);i++){
        buffPageLookUpTbl[i]=buffPageLookUpTbl[i+1];
    }
    buffPageLookUpTbl[bufferFrameSize-1]=firstFrame;
}

int getIndexPageByFIFO(){
    int indexFrame=-1;
    if(buffFrameCount<bufferFrameSize){
        for(int i=0; i<bufferFrameSize; i++){
            if(buffPageLookUpTbl[i]==-1){
                indexFrame=i;
                break;
            }
        }
    }
    if(indexFrame==-1)
        indexFrame =0;
    return indexFrame;
}

int isPagePresent(int pageNum){
    int frameIndex=-1;
    /*for(int i=0; i<bufferFrameSize; i++){
        printf(" \n buff %d",buffPageLookUpTbl[i]);
    }*/
    for(int i=0; i <bufferFrameSize; i++){
        if(buffPageLookUpTbl[i]==pageNum){
            frameIndex = i;
            break;
        }
    }
    return frameIndex;
}

int isPageDirty(PIN_PAGE * head,int frameIndex){
    PIN_PAGE *tempPage=head;
    int counter=0;
    while(counter<frameIndex){
        tempPage=tempPage->nxt_pin_page;
        counter++;
    }
    return tempPage->isDirty;
}

PIN_PAGE* getFrameFromLoc(PIN_PAGE * head, int frameIndex){
    PIN_PAGE *tempPage=head;
    int counter=0;
    while(counter<frameIndex){
        tempPage=tempPage->nxt_pin_page;
        counter++;
    }
    return tempPage;
}

PIN_PAGE* updateFixCountAtFrameLoc(PIN_PAGE *head,int frameIndex,int value){
    PIN_PAGE *tempPage=head;
    int counter=0;
    while(counter<frameIndex){
        tempPage=tempPage->nxt_pin_page;
        counter++;
    }
    tempPage->fixCount =  tempPage->fixCount + value;
    return tempPage;
}

RC displayBuffContent(PIN_PAGE *firstPage){
    PIN_PAGE *tempPage=firstPage;

    while(tempPage!=NULL){
        tempPage=tempPage->nxt_pin_page;
    }
}

RC testReadBlock(){
    ph = (SM_PageHandle)malloc(PAGE_SIZE);
    readBlock(0,fh,ph);
    for(int i=0; i<PAGE_SIZE; i++)
        printf("\n content read %c",ph[i]);
}

int* returnPagePosition(PIN_PAGE *head,int *loc){
    PIN_PAGE *tempPage = head;
    int pageLoc[bufferFrameSize];
    int count=0;
    while(tempPage!=NULL){
        *loc=tempPage->pageNum;
        tempPage=tempPage->nxt_pin_page;
        loc++;
        count++;
    }
    return loc;
}
