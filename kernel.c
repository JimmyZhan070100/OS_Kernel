#include "kernel.h"

PhysFrame *physFrame_head;
int free_frame_count = 0;
unsigned long next_PT_validAddr = VMEM_1_LIMIT - PAGESIZE;

unsigned int process_count = 0;
pcb *idle_proc;

unsigned long GetFreeFrame(){
    PhysFrame *tmp = physFrame_head;
    physFrame_head = tmp->next;
    unsigned long freeAddr = tmp->addr;
    free(tmp);
    free_frame_count--;
    return freeAddr;
}

unsigned long GetFromPTMEM(){
    unsigned long freeAddr = next_PT_validAddr;
    struct pte *pt1 = (struct pte *)ReadRegister(REG_PTR1);
    next_PT_validAddr -= PAGESIZE;
    int pageNumber = (freeAddr - VMEM_1_BASE) >> PAGESHIFT;
    if(pt1[pageNumber].valid == 0){
        pt1[pageNumber].valid = 1;
        pt1[pageNumber].kprot = PROT_READ | PROT_WRITE;
        pt1[pageNumber].uprot = PROT_NONE;
        pt1[pageNumber].pfn = GetFreeFrame();
        TracePrintf(6, "pt1[0x%lx].pfn = 0x%lx\n", pageNumber, pt1[pageNumber].pfn);
    }
    return freeAddr;
}

void FreeFrame(unsigned long addr){
    PhysFrame *new_frame = malloc(sizeof(PhysFrame));
    new_frame->addr = addr;
    new_frame->next = physFrame_head;
    physFrame_head = new_frame;
    free_frame_count++;
}
