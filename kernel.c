#include "kernel.h"

unsigned long TempVirtualMem;

PhysFrame *physFrame_head;
int free_frame_count = 0;
unsigned long next_PT_validAddr = VMEM_1_LIMIT - 2*PAGESIZE;

struct pte *pt_r1;
struct pte *pt_r0;

unsigned int process_count = 0;
pcb *active_proc;
pcb *idle_proc;
pcb *init_proc;
pcb delay_proc;

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
        TracePrintf(5, "pt1[0x%lx].pfn = 0x%lx\n", pageNumber, pt1[pageNumber].pfn);
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

void GetfreePhysicalAddr(unsigned long begin, unsigned long end, unsigned int kprot, unsigned uprot, struct pte* cur_ptr0){
    TracePrintf(0, "In GetfreePhysicalAddr function\n");
    // unsigned long new_brake = UP_TO_PAGE(end);
    // unsigned long old_brake = UP_TO_PAGE(begin);
    unsigned long page;
    struct pte* TempVir2phy = TempVirtualMem;
    pcb* p1pcb = (pcb *)(cur_ptr0);
    pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = (unsigned long)(cur_ptr0) >> PAGESHIFT;
    WriteRegister(REG_TLB_FLUSH, TempVirtualMem);
    for(page = begin; page < end; page++){
        TracePrintf(0, "Page address = 0x%lx\n", page);
        TempVir2phy[page].valid = 1;
        TracePrintf(0, "valid\n");
        TracePrintf(0, "TempVir2phy[page] = 0x%lx\n", TempVir2phy[page].valid);
        TempVir2phy[page].kprot = kprot;
        TempVir2phy[page].uprot = uprot;
        TempVir2phy[page].pfn = GetFreeFrame();
    }
}

void FreePhysicalAddr(unsigned long begin, unsigned long end, struct pte* cur_ptr0){
    TracePrintf(0, "In FreePhysicalAddr function\n");
    unsigned long new_brake = UP_TO_PAGE(end);
    unsigned long old_brake = UP_TO_PAGE(begin);
    unsigned long page;
    struct pte* TempVir2phy = TempVirtualMem;
    pcb* p1pcb = (pcb *)(cur_ptr0);
    pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = (unsigned long)(cur_ptr0) >> PAGESHIFT;
    for(page = old_brake; page < new_brake; page+=PAGESIZE){
        if(TempVir2phy[page].valid == 1){
            FreeFrame(TempVir2phy[page].pfn);
        }
        TempVir2phy[page].valid = 0;
    }
}

int CheckPhysFrame(int req_count){
    TracePrintf(0, "free_frame_count = %d vs %d\n", free_frame_count, req_count);
    return free_frame_count >= req_count;
}

/* PCB function */

void AddPCB(pcb *cur, pcb *que){
    while(que->next){
        que = que->next;
    }
    que->next = cur;
    cur->prev = que;
    cur->next = NULL;
}

pcb *PopPCB(pcb *que){
    pcb *temp = que->next;
    if(temp != NULL){
        que->next = temp->next;
        if(temp->next != NULL){
            temp->next->prev = que;
        }
        temp->next = NULL;
        temp->prev = NULL;
    }
    return temp;
}

void RemovePCB(pcb *cur){
    pcb *temp = cur->next;
    if(cur->prev != NULL){
        cur->prev->next = temp;
    }
    if(temp != NULL){
        temp->prev = cur->prev;
    }
    cur->next = NULL;
    cur->prev = NULL;
}

/* Swtich function*/

RCS421RegVal Virt2Phy(unsigned long addr){
    unsigned long pageNumber = (addr - VMEM_1_BASE) >> PAGESHIFT;
    struct pte *pt_r1 = (struct pte *)ReadRegister(REG_PTR1);
    return (RCS421RegVal)(pt_r1[pageNumber].pfn<<PAGESHIFT|(addr&PAGEOFFSET));
}

SavedContext *init_SwtichFunc(SavedContext *ctpx, void *p1, void *p2){
    TracePrintf(0, "In init_SwitchFunc from 0x%lx to 0x%lx\n", p1, p2);

    // Fork init process
    struct pte *init_pt_r0 = (struct pte *)(GetFreeFrame() << PAGESHIFT);
    TracePrintf(0, "init_pt_r0 = 0x%lx\n", init_pt_r0);
    // int index = PAGE_TABLE_LEN - 1;
    int index;
    struct pte* TempVir2phy = TempVirtualMem;
    pcb* p1pcb = (pcb *)(p1);
    struct pte* p1_ptr0 = p1pcb->pt_r0;
    for(index=0; index<PAGE_TABLE_LEN; index++){
        pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = (unsigned long)(init_pt_r0) >> PAGESHIFT;
        WriteRegister(REG_TLB_FLUSH, TempVirtualMem);
        // TracePrintf(0, "TempVirtualMem-VMEM_1_BASE = 0x%lx\n", TempVirtualMem-VMEM_1_BASE);
        if(p1_ptr0[index].valid){
            TracePrintf(0, "%d is valid\n", index);
            TempVir2phy[index].valid = p1_ptr0[index].valid;
            TempVir2phy[index].kprot = p1_ptr0[index].kprot;
            TempVir2phy[index].uprot = p1_ptr0[index].uprot;
            TempVir2phy[index].pfn = GetFreeFrame();
            TracePrintf(0, "TempVir2phy[index].pfn = 0x%lx\n", TempVir2phy[index].pfn);
            pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = TempVir2phy[index].pfn;
            WriteRegister(REG_TLB_FLUSH, TempVirtualMem);
            memcpy(TempVirtualMem, index*PAGESIZE, PAGESIZE);
        }
        else{
            // TracePrintf(0, "not valid\n");
            TempVir2phy[index].valid = 0;
        }
    }
    // for(i=0; i<KERNEL_STACK_PAGES; i++, index--){
    //     init_pt_r0[index].valid = 1;
    //     init_pt_r0[index].kprot = PROT_READ | PROT_WRITE;
    //     init_pt_r0[index].uprot = PROT_NONE;
    //     init_pt_r0[index].pfn = GetFreeFrame();
    //     TracePrintf(5, "init_pt_r0[0x%lx].pfn = 0x%lx\n", index, init_pt_r0[index].pfn);
    // }
    init_proc = (pcb*)malloc(sizeof(pcb));
    init_proc->pid = 1;
    init_proc->brk = MEM_INVALID_SIZE;
    init_proc->ctx = (SavedContext*)malloc(sizeof(SavedContext));
    init_proc->pt_r0 = init_pt_r0;
    pt_r0 = init_pt_r0;
    active_proc = init_proc;
    //

    // ((pcb *)p1)->ctx = ctpx;
    // void *kernel_stack = malloc(KERNEL_STACK_SIZE);
    // memcpy(kernel_stack, (void *)KERNEL_STACK_BASE, (size_t)KERNEL_STACK_SIZE);
    TracePrintf(0, "assign new_ptr0\n");
    // struct pte *new_ptr0 = ((pcb *)p2)->pt_r0;
    // WriteRegister(REG_PTR0, Virt2Phy((unsigned long)new_ptr0));
    TracePrintf(0, "Before Flush\n");
    WriteRegister(REG_PTR0, init_pt_r0);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    TracePrintf(0, "Finish Flush\n");
    // memcpy((void *)KERNEL_STACK_BASE, kernel_stack, (size_t)KERNEL_STACK_SIZE);
    // free(kernel_stack);
    return ctpx;
}

SavedContext *Delay_SwitchFunc(SavedContext *ctpx, void *p1, void *p2){
    TracePrintf(0, "In Delay_SwitchFunc from 0x%lx to 0x%lx\n", p1, p2);
    if(p2 == NULL){
        WriteRegister(REG_PTR0, Virt2Phy((unsigned long)idle_proc));
        active_proc = &idle_proc;
    }
    else{
        WriteRegister(REG_PTR0, Virt2Phy((unsigned long)((pcb *)p2)->pt_r0));
        RemovePCB(p2);
        active_proc = p2;
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    AddPCB(p1, &delay_proc);
    return &active_proc->ctx;
}