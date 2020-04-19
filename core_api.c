/* 046267 Computer Architecture - Winter 2019/20 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>

typedef enum MT_type {BLOCKING, FG} type;

typedef struct thread{
    tcontext threadRegs;
    uint32_t currentPc;
    unsigned whenReady;
    bool halt;
} Thread;

typedef struct mt{
    int currentCycle;
    int numOfInst;
    Thread * threadsArray;
    int numOfThreads;
    type mtType;
    int penaltyCycles;
    int currentThread;
} MT;


MT * globalMT;

int  nextReadyThread(bool isIdle){
    int nextReadyThread = globalMT->currentThread;
    // int counter = 0; // to make sure we are checking the arr only once;
    int i;
    int p;
    int counter=0;
    if(isIdle) {
        if(globalMT->mtType == BLOCKING )
            p=nextReadyThread;
        else{
            if (nextReadyThread < (globalMT->numOfThreads - 1))
                p = nextReadyThread + 1;
            else
                p = 0;
        }
    }
    else {
        if (nextReadyThread < (globalMT->numOfThreads - 1))
            p = nextReadyThread + 1;
        else
            p = 0;
    }
    for( i=p ;i<globalMT->numOfThreads;i++) {

        //   if(counter == globalMT->numOfThreads)//avoiding endless searching
        //    return -1;
        //counter++;
        if(!globalMT->threadsArray[i].halt && globalMT->threadsArray[i].whenReady<=globalMT->currentCycle)
            return i;
        if(i == globalMT->numOfThreads - 1) //because we are looking for ready thread in a cyclic way
        {
            for(int j=0;j<=globalMT->currentThread;j++) {
                if (!globalMT->threadsArray[j].halt &&
                    globalMT->threadsArray[j].whenReady <= globalMT->currentCycle)
                    return j;
            }
            return -1;
        }
    }


}
void loadExecute(Instruction currentInst){
    uint32_t src1;
    uint32_t src2;
    int32_t dst;
    src1=globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.src1_index];
    if(!currentInst.isSrc2Imm)
        src2=globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.src2_index_imm];
    else
        src2=currentInst.src2_index_imm;
    SIM_MemDataRead(src1+src2,  &dst);
    globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.dst_index]=dst;

}

void storeExecute(Instruction currentInst){
    int32_t src1;
    uint32_t src2;
    uint32_t dst;
    /* SIM_MemDataRead(globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.src1_index],&src1);
     SIM_MemDataRead(globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.dst_index],&dst);
     if(!currentInst.src2_index_imm){
         SIM_MemDataRead(globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.src2_index_imm],&src2);
     }*/
    dst=globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.dst_index];
    src1=globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.src1_index];
    if(!currentInst.isSrc2Imm)
        src2=globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.src2_index_imm];
    else
        src2=currentInst.src2_index_imm;
    SIM_MemDataWrite(src2+dst,src1);
}


void executeCmd(Instruction currentInst){
    int32_t src1;
    int32_t src2;
    int32_t dst;
    src1=globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.src1_index];
    if(!currentInst.isSrc2Imm)
        src2=globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.src2_index_imm];
    else
        src2=currentInst.src2_index_imm;
    cmd_opcode currentOp = currentInst.opcode;
    if(currentOp==CMD_ADD || currentOp==CMD_ADDI)
        globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.dst_index]=src1+src2;
    if(currentOp==CMD_SUB||currentOp==CMD_SUBI)
        globalMT->threadsArray[globalMT->currentThread].threadRegs.reg[currentInst.dst_index]=src1-src2;

}


void mt_create(int numOfThreads, type type, int penalty){
    globalMT = (MT*)malloc(sizeof(MT));
    if(!globalMT){
        return;
    }
    globalMT->currentThread = 0;
    globalMT->currentCycle = 0;
    globalMT->numOfInst=0;
    globalMT->mtType=type;
    globalMT->penaltyCycles=penalty;
    globalMT->numOfThreads=numOfThreads;
    globalMT->threadsArray=(Thread*)malloc(sizeof(Thread)*numOfThreads);
    if(!globalMT->threadsArray){
        free(globalMT);
        return;
    }
    for (int i = 0; i < numOfThreads ; ++i) {
        globalMT->threadsArray[i].currentPc=0;
        globalMT->threadsArray[i].whenReady=0;
        globalMT->threadsArray[i].halt=false;
        for (int j = 0; j < REGS_COUNT; ++j) {
            globalMT->threadsArray[i].threadRegs.reg[j]=0;
        }
    }
}


bool checkIfAllFinish(){
    for (int i = 0; i < globalMT->numOfThreads; ++i) {
        if (globalMT->threadsArray[i].halt==false) {
            return false;
        }
    }
    return true;
}


void CORE_BlockedMT() {
    mt_create(SIM_GetThreadsNum(),BLOCKING,SIM_GetSwitchCycles());
    bool allFinish = false;
    bool contextSwitch=false;
    int nextThread =0;
    bool isIdle=false;
    while (!allFinish){
        int tempThread=globalMT->currentThread;
        if(contextSwitch || isIdle)
            nextThread = nextReadyThread(isIdle);

        if(tempThread!=nextThread && nextThread!=-1){
            globalMT->currentCycle+=globalMT->penaltyCycles;
        }

        contextSwitch = false;
        Instruction currentInst;
        if(nextThread>=0) {

            //if(isIdle && nextThread== globalMT->currentThread) //isIdle for the previous cycle
            //   globalMT->currentCycle-=globalMT->penaltyCycles;
            isIdle=false;
            globalMT->currentThread = nextThread;
            globalMT->numOfInst++;
            SIM_MemInstRead(globalMT->threadsArray[nextThread].currentPc, &currentInst, nextThread);

            globalMT->threadsArray[nextThread].currentPc++;
            if(currentInst.opcode==CMD_LOAD){
                loadExecute(currentInst);
                contextSwitch=true;
                globalMT->threadsArray[nextThread].whenReady=globalMT->currentCycle+SIM_GetLoadLat()+1;
            }
            else if(currentInst.opcode==CMD_STORE ){
                storeExecute(currentInst);
                contextSwitch=true;
                globalMT->threadsArray[nextThread].whenReady=globalMT->currentCycle+SIM_GetStoreLat()+1;
            }
            else if(currentInst.opcode==CMD_HALT ){
                contextSwitch=true;
                globalMT->threadsArray[nextThread].halt=true;
            }
            else{
                contextSwitch=false;
                executeCmd(currentInst);
                globalMT->threadsArray[nextThread].whenReady=globalMT->currentCycle+1;
            }
        }
        else{
            isIdle=true;

        }
        allFinish=checkIfAllFinish();
        /*  if(!allFinish && contextSwitch){
              globalMT->currentCycle+=globalMT->penaltyCycles;
          }*/
        globalMT->currentCycle++;

    }
}

void CORE_FinegrainedMT() {
    mt_create(SIM_GetThreadsNum(),FG,0);
    bool allFinish = false;
    int nextThread =0;
    int flag=0;
    bool isIdle=true;
    Instruction currentInst;
    while(!allFinish){
        if(flag == 0) {//only for the first time
            nextThread = 0;
            flag++;
        }
        else {
            nextThread = nextReadyThread(isIdle);
        }
        //printf("thread num %d \n",nextThread);
        allFinish=checkIfAllFinish();
        if(nextThread>=0){
            isIdle=false;
            globalMT->currentThread = nextThread;
            globalMT->numOfInst++;
            SIM_MemInstRead(globalMT->threadsArray[nextThread].currentPc, &currentInst, nextThread);
            globalMT->threadsArray[nextThread].currentPc++;
            if(currentInst.opcode==CMD_LOAD){
                loadExecute(currentInst);
                globalMT->threadsArray[nextThread].whenReady=globalMT->currentCycle+SIM_GetLoadLat()+1;
                globalMT->threadsArray[nextThread].whenReady=globalMT->threadsArray[nextThread].whenReady;
            }
            else if(currentInst.opcode==CMD_STORE ){
                storeExecute(currentInst);
                globalMT->threadsArray[nextThread].whenReady=globalMT->currentCycle+SIM_GetStoreLat()+1;
            }
            else if(currentInst.opcode==CMD_HALT ){
                globalMT->threadsArray[nextThread].halt=true;
            }
            else{
                executeCmd(currentInst);
                globalMT->threadsArray[nextThread].whenReady=globalMT->currentCycle+1;
            }
        }
        else{
            isIdle=true;
        }
        allFinish=checkIfAllFinish();
        globalMT->currentCycle++;
    }
}



double CORE_BlockedMT_CPI(){
    free(globalMT->threadsArray);
    if(globalMT->numOfInst>0){
        double cpi= (double)(globalMT->currentCycle)/(double)(globalMT->numOfInst);
        free(globalMT);
        return cpi;
    }
    free(globalMT);
    return 0;
}

double CORE_FinegrainedMT_CPI(){
    free(globalMT->threadsArray);
    if(globalMT->numOfInst>0){
        double cpi= (double)(globalMT->currentCycle)/(double)(globalMT->numOfInst);
        free(globalMT);
        return cpi;
    }
    free(globalMT);
    return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {

    for (int i = 0; i < REGS_COUNT; ++i) {
        context[threadid].reg[i]=globalMT->threadsArray[threadid].threadRegs.reg[i];
    }
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
    for (int i = 0; i < REGS_COUNT; ++i) {
        context[threadid].reg[i]=globalMT->threadsArray[threadid].threadRegs.reg[i];
    }
}

